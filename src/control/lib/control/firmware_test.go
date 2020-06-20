//
// (C) Copyright 2020 Intel Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
// The Government's rights to use, modify, reproduce, release, perform, display,
// or disclose this software are subject to the terms of the Apache License as
// provided in Contract No. 8F-30005.
// Any reproduction of computer software, computer software documentation, or
// portions thereof marked with this legend must also reproduce the markings.
//
// +build firmware

package control

import (
	"context"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/pkg/errors"

	"github.com/daos-stack/daos/src/control/common"
	ctlpb "github.com/daos-stack/daos/src/control/common/proto/ctl"
	"github.com/daos-stack/daos/src/control/lib/hostlist"
	"github.com/daos-stack/daos/src/control/logging"
	"github.com/daos-stack/daos/src/control/server/storage"
)

func createTestHostSet(t *testing.T, hosts string) *hostlist.HostSet {
	set, err := hostlist.CreateSet(hosts)
	if err != nil {
		t.Fatalf("couldn't create host set: %s", err)
	}
	return set
}

func TestControl_FirmwareQuery(t *testing.T) {
	pbResults := []*ctlpb.ScmFirmwareQueryResp{
		{
			Uid:               "TestUid1",
			Handle:            1,
			ActiveVersion:     "ACTIVE1",
			StagedVersion:     "STAGED",
			ImageMaxSizeBytes: 3200,
			UpdateStatus:      uint32(storage.ScmUpdateStatusStaged),
		},
		{
			Uid:               "TestUid2",
			Handle:            2,
			ActiveVersion:     "ACTIVE2",
			StagedVersion:     "",
			ImageMaxSizeBytes: 6400,
			UpdateStatus:      uint32(storage.ScmUpdateStatusSuccess),
		},
		{
			Uid:    "TestUid3",
			Handle: 3,
			Error:  "Failed getting firmware info",
		},
	}

	expResults := make([]*SCMFirmwareQueryResult, 0, len(pbResults))
	for _, pbRes := range pbResults {
		res := &SCMFirmwareQueryResult{
			DeviceUID:    pbRes.Uid,
			DeviceHandle: pbRes.Handle,
			Info: storage.ScmFirmwareInfo{
				ActiveVersion:     pbRes.ActiveVersion,
				StagedVersion:     pbRes.StagedVersion,
				ImageMaxSizeBytes: pbRes.ImageMaxSizeBytes,
				UpdateStatus:      storage.ScmFirmwareUpdateStatus(pbRes.UpdateStatus),
			},
		}

		if pbRes.Error != "" {
			res.Error = errors.New(pbRes.Error)
		}

		expResults = append(expResults, res)
	}

	for name, tc := range map[string]struct {
		mic     *MockInvokerConfig
		req     *FirmwareQueryReq
		expResp *FirmwareQueryResp
		expErr  error
	}{
		"nothing requested": {
			req:    &FirmwareQueryReq{},
			expErr: errors.New("no device types requested"),
		},
		"local failure": {
			req: &FirmwareQueryReq{SCM: true},
			mic: &MockInvokerConfig{
				UnaryError: errors.New("local failed"),
			},
			expErr: errors.New("local failed"),
		},
		"remote failure": {
			req: &FirmwareQueryReq{SCM: true},
			mic: &MockInvokerConfig{
				UnaryResponse: MockMSResponse("host1", errors.New("remote failed"), nil),
			},
			expResp: &FirmwareQueryResp{
				HostErrorsResp: HostErrorsResp{
					HostErrors: HostErrorsMap{
						"remote failed": &HostErrorSet{
							HostSet:   createTestHostSet(t, "host1"),
							HostError: errors.New("remote failed"),
						},
					},
				},
			},
		},
		"SCM success": {
			req: &FirmwareQueryReq{SCM: true},
			mic: &MockInvokerConfig{
				UnaryResponse: MockMSResponse("host1", nil, &ctlpb.FirmwareQueryResp{
					ScmResults: pbResults,
				}),
			},
			expResp: &FirmwareQueryResp{
				HostSCMFirmware: map[string][]*SCMFirmwareQueryResult{
					"host1": expResults,
				},
			},
		},
	} {
		t.Run(name, func(t *testing.T) {
			log, buf := logging.NewTestLogger(t.Name())
			defer common.ShowBufferOnFailure(t, buf)

			mic := tc.mic
			if mic == nil {
				mic = DefaultMockInvokerConfig()
			}

			ctx := context.TODO()
			mi := NewMockInvoker(log, mic)

			gotResp, gotErr := FirmwareQuery(ctx, mi, tc.req)
			common.CmpErr(t, tc.expErr, gotErr)
			if tc.expErr != nil {
				return
			}

			cmpOpts := []cmp.Option{
				cmp.Comparer(func(e1, e2 error) bool {
					if e1 == e2 {
						return true
					}
					if e1 == nil || e2 == nil {
						return false
					}
					if e1.Error() == e2.Error() {
						return true
					}
					return false
				}),
				cmp.Comparer(func(h1, h2 hostlist.HostSet) bool {
					if h1.String() == h2.String() {
						return true
					}
					return false
				}),
			}
			if diff := cmp.Diff(tc.expResp, gotResp, cmpOpts...); diff != "" {
				t.Fatalf("Unexpected response (-want, +got):\n%s\n", diff)
			}
		})
	}
}
