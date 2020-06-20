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

package server

import (
	"context"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/pkg/errors"

	"github.com/daos-stack/daos/src/control/common"
	ctlpb "github.com/daos-stack/daos/src/control/common/proto/ctl"
	"github.com/daos-stack/daos/src/control/logging"
	"github.com/daos-stack/daos/src/control/server/storage"
	"github.com/daos-stack/daos/src/control/server/storage/bdev"
	"github.com/daos-stack/daos/src/control/server/storage/scm"
)

func TestCtlSvc_FirmwareQuery(t *testing.T) {
	testFWInfo := &storage.ScmFirmwareInfo{
		ActiveVersion:     "MyActiveVersion",
		StagedVersion:     "MyStagedVersion",
		ImageMaxSizeBytes: 1024,
		UpdateStatus:      storage.ScmUpdateStatusStaged,
	}

	for name, tc := range map[string]struct {
		bmbc    *bdev.MockBackendConfig
		smbc    *scm.MockBackendConfig
		req     ctlpb.FirmwareQueryReq
		expErr  error
		expResp *ctlpb.FirmwareQueryResp
	}{
		"nothing requested": {
			expResp: &ctlpb.FirmwareQueryResp{},
		},
		"SCM - query failed": {
			req: ctlpb.FirmwareQueryReq{
				QueryScm: true,
			},
			smbc: &scm.MockBackendConfig{
				DiscoverErr: errors.New("mock discovery failed"),
			},
			expErr: errors.New("mock discovery failed"),
		},
		"SCM - no devices": {
			req: ctlpb.FirmwareQueryReq{
				QueryScm: true,
			},
			smbc: &scm.MockBackendConfig{},
			expResp: &ctlpb.FirmwareQueryResp{
				ScmResults: []*ctlpb.ScmFirmwareQueryResp{},
			},
		},
		"SCM - success with devices": {
			req: ctlpb.FirmwareQueryReq{
				QueryScm: true,
			},
			smbc: &scm.MockBackendConfig{
				DiscoverRes: storage.ScmModules{
					{UID: "TestUid1"},
					{UID: "TestUid2"},
					{UID: "TestUid3"},
				},
				GetFirmwareStatusRes: testFWInfo,
			},
			expResp: &ctlpb.FirmwareQueryResp{
				ScmResults: []*ctlpb.ScmFirmwareQueryResp{
					{
						Uid:               "TestUid1",
						ActiveVersion:     testFWInfo.ActiveVersion,
						StagedVersion:     testFWInfo.StagedVersion,
						ImageMaxSizeBytes: testFWInfo.ImageMaxSizeBytes,
						UpdateStatus:      uint32(testFWInfo.UpdateStatus),
					},
					{
						Uid:               "TestUid2",
						ActiveVersion:     testFWInfo.ActiveVersion,
						StagedVersion:     testFWInfo.StagedVersion,
						ImageMaxSizeBytes: testFWInfo.ImageMaxSizeBytes,
						UpdateStatus:      uint32(testFWInfo.UpdateStatus),
					},
					{
						Uid:               "TestUid3",
						ActiveVersion:     testFWInfo.ActiveVersion,
						StagedVersion:     testFWInfo.StagedVersion,
						ImageMaxSizeBytes: testFWInfo.ImageMaxSizeBytes,
						UpdateStatus:      uint32(testFWInfo.UpdateStatus),
					},
				},
			},
		},
	} {
		t.Run(name, func(t *testing.T) {
			log, buf := logging.NewTestLogger(t.Name())
			defer common.ShowBufferOnFailure(t, buf)

			config := emptyMockConfig(t)
			cs := mockControlService(t, log, config, tc.bmbc, tc.smbc, nil)

			resp, err := cs.FirmwareQuery(context.TODO(), &tc.req)

			common.CmpErr(t, tc.expErr, err)

			if diff := cmp.Diff(tc.expResp, resp); diff != "" {
				t.Fatalf("unexpected response (-want, +got):\n%s\n", diff)
			}
		})
	}
}
