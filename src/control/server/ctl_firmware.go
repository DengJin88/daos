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

	ctlpb "github.com/daos-stack/daos/src/control/common/proto/ctl"
	"github.com/daos-stack/daos/src/control/server/storage/scm"
)

// FirmwareQuery implements the method defined for the control service if
// firmware management is enabled for this build.
//
// It fetches information about the device firmware on this server based on the
// caller's request parameters. It can fetch firmware information for NVMe, SCM,
// or both.
func (svc *ControlService) FirmwareQuery(parent context.Context, pbReq *ctlpb.FirmwareQueryReq) (*ctlpb.FirmwareQueryResp, error) {
	svc.log.Debug("received FirmwareQuery RPC")

	pbResp := new(ctlpb.FirmwareQueryResp)

	if pbReq.QueryScm {
		queryResp, err := svc.scm.QueryFirmware(scm.FirmwareQueryRequest{})
		if err != nil {
			return nil, err
		}

		pbResp.ScmResults = make([]*ctlpb.ScmFirmwareQueryResp, 0, len(queryResp.FirmwareInfo))
		for uid, info := range queryResp.FirmwareInfo {
			pbResult := &ctlpb.ScmFirmwareQueryResp{
				Uid:               uid,
				ActiveVersion:     info.ActiveVersion,
				StagedVersion:     info.StagedVersion,
				ImageMaxSizeBytes: info.ImageMaxSizeBytes,
				UpdateStatus:      uint32(info.UpdateStatus),
			}
			pbResp.ScmResults = append(pbResp.ScmResults, pbResult)
		}
	}

	c.log.Debug("responding to FirmwareQuery RPC")
	return pbResp, nil
}
