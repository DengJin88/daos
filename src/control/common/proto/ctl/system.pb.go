// Code generated by protoc-gen-go. DO NOT EDIT.
// source: system.proto

package ctl

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion2 // please upgrade the proto package

// SystemMember refers to a data-plane instance that is a member of DAOS
// system running on host with the control-plane listening at "Addr".
type SystemMember struct {
	Addr  string `protobuf:"bytes,1,opt,name=addr,proto3" json:"addr,omitempty"`
	Uuid  string `protobuf:"bytes,2,opt,name=uuid,proto3" json:"uuid,omitempty"`
	Rank  uint32 `protobuf:"varint,3,opt,name=rank,proto3" json:"rank,omitempty"`
	State uint32 `protobuf:"varint,4,opt,name=state,proto3" json:"state,omitempty"`
	// ancillary info e.g. error msg or reason for state change
	Info                 string   `protobuf:"bytes,5,opt,name=info,proto3" json:"info,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *SystemMember) Reset()         { *m = SystemMember{} }
func (m *SystemMember) String() string { return proto.CompactTextString(m) }
func (*SystemMember) ProtoMessage()    {}
func (*SystemMember) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{0}
}
func (m *SystemMember) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemMember.Unmarshal(m, b)
}
func (m *SystemMember) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemMember.Marshal(b, m, deterministic)
}
func (dst *SystemMember) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemMember.Merge(dst, src)
}
func (m *SystemMember) XXX_Size() int {
	return xxx_messageInfo_SystemMember.Size(m)
}
func (m *SystemMember) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemMember.DiscardUnknown(m)
}

var xxx_messageInfo_SystemMember proto.InternalMessageInfo

func (m *SystemMember) GetAddr() string {
	if m != nil {
		return m.Addr
	}
	return ""
}

func (m *SystemMember) GetUuid() string {
	if m != nil {
		return m.Uuid
	}
	return ""
}

func (m *SystemMember) GetRank() uint32 {
	if m != nil {
		return m.Rank
	}
	return 0
}

func (m *SystemMember) GetState() uint32 {
	if m != nil {
		return m.State
	}
	return 0
}

func (m *SystemMember) GetInfo() string {
	if m != nil {
		return m.Info
	}
	return ""
}

// RankResult is a generic result for a system operation on a rank.
// Identical to mgmt.proto RanksResp_RankResult.
type RankResult struct {
	Rank                 uint32   `protobuf:"varint,1,opt,name=rank,proto3" json:"rank,omitempty"`
	Action               string   `protobuf:"bytes,2,opt,name=action,proto3" json:"action,omitempty"`
	Errored              bool     `protobuf:"varint,3,opt,name=errored,proto3" json:"errored,omitempty"`
	Msg                  string   `protobuf:"bytes,4,opt,name=msg,proto3" json:"msg,omitempty"`
	State                uint32   `protobuf:"varint,5,opt,name=state,proto3" json:"state,omitempty"`
	Addr                 string   `protobuf:"bytes,6,opt,name=addr,proto3" json:"addr,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *RankResult) Reset()         { *m = RankResult{} }
func (m *RankResult) String() string { return proto.CompactTextString(m) }
func (*RankResult) ProtoMessage()    {}
func (*RankResult) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{1}
}
func (m *RankResult) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_RankResult.Unmarshal(m, b)
}
func (m *RankResult) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_RankResult.Marshal(b, m, deterministic)
}
func (dst *RankResult) XXX_Merge(src proto.Message) {
	xxx_messageInfo_RankResult.Merge(dst, src)
}
func (m *RankResult) XXX_Size() int {
	return xxx_messageInfo_RankResult.Size(m)
}
func (m *RankResult) XXX_DiscardUnknown() {
	xxx_messageInfo_RankResult.DiscardUnknown(m)
}

var xxx_messageInfo_RankResult proto.InternalMessageInfo

func (m *RankResult) GetRank() uint32 {
	if m != nil {
		return m.Rank
	}
	return 0
}

func (m *RankResult) GetAction() string {
	if m != nil {
		return m.Action
	}
	return ""
}

func (m *RankResult) GetErrored() bool {
	if m != nil {
		return m.Errored
	}
	return false
}

func (m *RankResult) GetMsg() string {
	if m != nil {
		return m.Msg
	}
	return ""
}

func (m *RankResult) GetState() uint32 {
	if m != nil {
		return m.State
	}
	return 0
}

func (m *RankResult) GetAddr() string {
	if m != nil {
		return m.Addr
	}
	return ""
}

// SystemStopReq supplies system shutdown parameters.
type SystemStopReq struct {
	Prep                 bool     `protobuf:"varint,1,opt,name=prep,proto3" json:"prep,omitempty"`
	Kill                 bool     `protobuf:"varint,2,opt,name=kill,proto3" json:"kill,omitempty"`
	Force                bool     `protobuf:"varint,3,opt,name=force,proto3" json:"force,omitempty"`
	Ranks                []uint32 `protobuf:"varint,4,rep,packed,name=ranks,proto3" json:"ranks,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *SystemStopReq) Reset()         { *m = SystemStopReq{} }
func (m *SystemStopReq) String() string { return proto.CompactTextString(m) }
func (*SystemStopReq) ProtoMessage()    {}
func (*SystemStopReq) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{2}
}
func (m *SystemStopReq) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemStopReq.Unmarshal(m, b)
}
func (m *SystemStopReq) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemStopReq.Marshal(b, m, deterministic)
}
func (dst *SystemStopReq) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemStopReq.Merge(dst, src)
}
func (m *SystemStopReq) XXX_Size() int {
	return xxx_messageInfo_SystemStopReq.Size(m)
}
func (m *SystemStopReq) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemStopReq.DiscardUnknown(m)
}

var xxx_messageInfo_SystemStopReq proto.InternalMessageInfo

func (m *SystemStopReq) GetPrep() bool {
	if m != nil {
		return m.Prep
	}
	return false
}

func (m *SystemStopReq) GetKill() bool {
	if m != nil {
		return m.Kill
	}
	return false
}

func (m *SystemStopReq) GetForce() bool {
	if m != nil {
		return m.Force
	}
	return false
}

func (m *SystemStopReq) GetRanks() []uint32 {
	if m != nil {
		return m.Ranks
	}
	return nil
}

// SystemStopResp returns status of shutdown attempt and results
// of attempts to stop system members.
type SystemStopResp struct {
	Results              []*RankResult `protobuf:"bytes,1,rep,name=results,proto3" json:"results,omitempty"`
	XXX_NoUnkeyedLiteral struct{}      `json:"-"`
	XXX_unrecognized     []byte        `json:"-"`
	XXX_sizecache        int32         `json:"-"`
}

func (m *SystemStopResp) Reset()         { *m = SystemStopResp{} }
func (m *SystemStopResp) String() string { return proto.CompactTextString(m) }
func (*SystemStopResp) ProtoMessage()    {}
func (*SystemStopResp) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{3}
}
func (m *SystemStopResp) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemStopResp.Unmarshal(m, b)
}
func (m *SystemStopResp) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemStopResp.Marshal(b, m, deterministic)
}
func (dst *SystemStopResp) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemStopResp.Merge(dst, src)
}
func (m *SystemStopResp) XXX_Size() int {
	return xxx_messageInfo_SystemStopResp.Size(m)
}
func (m *SystemStopResp) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemStopResp.DiscardUnknown(m)
}

var xxx_messageInfo_SystemStopResp proto.InternalMessageInfo

func (m *SystemStopResp) GetResults() []*RankResult {
	if m != nil {
		return m.Results
	}
	return nil
}

// SystemResetFormatReq supplies system reset format parameters.
type SystemResetFormatReq struct {
	Ranks                []uint32 `protobuf:"varint,1,rep,packed,name=ranks,proto3" json:"ranks,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *SystemResetFormatReq) Reset()         { *m = SystemResetFormatReq{} }
func (m *SystemResetFormatReq) String() string { return proto.CompactTextString(m) }
func (*SystemResetFormatReq) ProtoMessage()    {}
func (*SystemResetFormatReq) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{4}
}
func (m *SystemResetFormatReq) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemResetFormatReq.Unmarshal(m, b)
}
func (m *SystemResetFormatReq) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemResetFormatReq.Marshal(b, m, deterministic)
}
func (dst *SystemResetFormatReq) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemResetFormatReq.Merge(dst, src)
}
func (m *SystemResetFormatReq) XXX_Size() int {
	return xxx_messageInfo_SystemResetFormatReq.Size(m)
}
func (m *SystemResetFormatReq) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemResetFormatReq.DiscardUnknown(m)
}

var xxx_messageInfo_SystemResetFormatReq proto.InternalMessageInfo

func (m *SystemResetFormatReq) GetRanks() []uint32 {
	if m != nil {
		return m.Ranks
	}
	return nil
}

// SystemResetFormatResp returns status of reset format attempt and results
// of attempts to reset format of system members.
type SystemResetFormatResp struct {
	Results              []*RankResult `protobuf:"bytes,1,rep,name=results,proto3" json:"results,omitempty"`
	XXX_NoUnkeyedLiteral struct{}      `json:"-"`
	XXX_unrecognized     []byte        `json:"-"`
	XXX_sizecache        int32         `json:"-"`
}

func (m *SystemResetFormatResp) Reset()         { *m = SystemResetFormatResp{} }
func (m *SystemResetFormatResp) String() string { return proto.CompactTextString(m) }
func (*SystemResetFormatResp) ProtoMessage()    {}
func (*SystemResetFormatResp) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{5}
}
func (m *SystemResetFormatResp) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemResetFormatResp.Unmarshal(m, b)
}
func (m *SystemResetFormatResp) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemResetFormatResp.Marshal(b, m, deterministic)
}
func (dst *SystemResetFormatResp) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemResetFormatResp.Merge(dst, src)
}
func (m *SystemResetFormatResp) XXX_Size() int {
	return xxx_messageInfo_SystemResetFormatResp.Size(m)
}
func (m *SystemResetFormatResp) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemResetFormatResp.DiscardUnknown(m)
}

var xxx_messageInfo_SystemResetFormatResp proto.InternalMessageInfo

func (m *SystemResetFormatResp) GetResults() []*RankResult {
	if m != nil {
		return m.Results
	}
	return nil
}

// SystemStartReq supplies system restart parameters.
type SystemStartReq struct {
	Ranks                []uint32 `protobuf:"varint,1,rep,packed,name=ranks,proto3" json:"ranks,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *SystemStartReq) Reset()         { *m = SystemStartReq{} }
func (m *SystemStartReq) String() string { return proto.CompactTextString(m) }
func (*SystemStartReq) ProtoMessage()    {}
func (*SystemStartReq) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{6}
}
func (m *SystemStartReq) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemStartReq.Unmarshal(m, b)
}
func (m *SystemStartReq) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemStartReq.Marshal(b, m, deterministic)
}
func (dst *SystemStartReq) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemStartReq.Merge(dst, src)
}
func (m *SystemStartReq) XXX_Size() int {
	return xxx_messageInfo_SystemStartReq.Size(m)
}
func (m *SystemStartReq) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemStartReq.DiscardUnknown(m)
}

var xxx_messageInfo_SystemStartReq proto.InternalMessageInfo

func (m *SystemStartReq) GetRanks() []uint32 {
	if m != nil {
		return m.Ranks
	}
	return nil
}

// SystemStartResp returns status of restart attempt and results
// of attempts to start system members.
type SystemStartResp struct {
	Results              []*RankResult `protobuf:"bytes,1,rep,name=results,proto3" json:"results,omitempty"`
	XXX_NoUnkeyedLiteral struct{}      `json:"-"`
	XXX_unrecognized     []byte        `json:"-"`
	XXX_sizecache        int32         `json:"-"`
}

func (m *SystemStartResp) Reset()         { *m = SystemStartResp{} }
func (m *SystemStartResp) String() string { return proto.CompactTextString(m) }
func (*SystemStartResp) ProtoMessage()    {}
func (*SystemStartResp) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{7}
}
func (m *SystemStartResp) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemStartResp.Unmarshal(m, b)
}
func (m *SystemStartResp) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemStartResp.Marshal(b, m, deterministic)
}
func (dst *SystemStartResp) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemStartResp.Merge(dst, src)
}
func (m *SystemStartResp) XXX_Size() int {
	return xxx_messageInfo_SystemStartResp.Size(m)
}
func (m *SystemStartResp) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemStartResp.DiscardUnknown(m)
}

var xxx_messageInfo_SystemStartResp proto.InternalMessageInfo

func (m *SystemStartResp) GetResults() []*RankResult {
	if m != nil {
		return m.Results
	}
	return nil
}

// SystemQueryReq supplies system query parameters.
type SystemQueryReq struct {
	Ranks                []uint32 `protobuf:"varint,1,rep,packed,name=ranks,proto3" json:"ranks,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *SystemQueryReq) Reset()         { *m = SystemQueryReq{} }
func (m *SystemQueryReq) String() string { return proto.CompactTextString(m) }
func (*SystemQueryReq) ProtoMessage()    {}
func (*SystemQueryReq) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{8}
}
func (m *SystemQueryReq) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemQueryReq.Unmarshal(m, b)
}
func (m *SystemQueryReq) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemQueryReq.Marshal(b, m, deterministic)
}
func (dst *SystemQueryReq) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemQueryReq.Merge(dst, src)
}
func (m *SystemQueryReq) XXX_Size() int {
	return xxx_messageInfo_SystemQueryReq.Size(m)
}
func (m *SystemQueryReq) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemQueryReq.DiscardUnknown(m)
}

var xxx_messageInfo_SystemQueryReq proto.InternalMessageInfo

func (m *SystemQueryReq) GetRanks() []uint32 {
	if m != nil {
		return m.Ranks
	}
	return nil
}

// SystemQueryResp returns active system members.
type SystemQueryResp struct {
	Members              []*SystemMember `protobuf:"bytes,1,rep,name=members,proto3" json:"members,omitempty"`
	XXX_NoUnkeyedLiteral struct{}        `json:"-"`
	XXX_unrecognized     []byte          `json:"-"`
	XXX_sizecache        int32           `json:"-"`
}

func (m *SystemQueryResp) Reset()         { *m = SystemQueryResp{} }
func (m *SystemQueryResp) String() string { return proto.CompactTextString(m) }
func (*SystemQueryResp) ProtoMessage()    {}
func (*SystemQueryResp) Descriptor() ([]byte, []int) {
	return fileDescriptor_system_6ffe04b71a69ff79, []int{9}
}
func (m *SystemQueryResp) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_SystemQueryResp.Unmarshal(m, b)
}
func (m *SystemQueryResp) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_SystemQueryResp.Marshal(b, m, deterministic)
}
func (dst *SystemQueryResp) XXX_Merge(src proto.Message) {
	xxx_messageInfo_SystemQueryResp.Merge(dst, src)
}
func (m *SystemQueryResp) XXX_Size() int {
	return xxx_messageInfo_SystemQueryResp.Size(m)
}
func (m *SystemQueryResp) XXX_DiscardUnknown() {
	xxx_messageInfo_SystemQueryResp.DiscardUnknown(m)
}

var xxx_messageInfo_SystemQueryResp proto.InternalMessageInfo

func (m *SystemQueryResp) GetMembers() []*SystemMember {
	if m != nil {
		return m.Members
	}
	return nil
}

func init() {
	proto.RegisterType((*SystemMember)(nil), "ctl.SystemMember")
	proto.RegisterType((*RankResult)(nil), "ctl.RankResult")
	proto.RegisterType((*SystemStopReq)(nil), "ctl.SystemStopReq")
	proto.RegisterType((*SystemStopResp)(nil), "ctl.SystemStopResp")
	proto.RegisterType((*SystemResetFormatReq)(nil), "ctl.SystemResetFormatReq")
	proto.RegisterType((*SystemResetFormatResp)(nil), "ctl.SystemResetFormatResp")
	proto.RegisterType((*SystemStartReq)(nil), "ctl.SystemStartReq")
	proto.RegisterType((*SystemStartResp)(nil), "ctl.SystemStartResp")
	proto.RegisterType((*SystemQueryReq)(nil), "ctl.SystemQueryReq")
	proto.RegisterType((*SystemQueryResp)(nil), "ctl.SystemQueryResp")
}

func init() { proto.RegisterFile("system.proto", fileDescriptor_system_6ffe04b71a69ff79) }

var fileDescriptor_system_6ffe04b71a69ff79 = []byte{
	// 354 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x9c, 0x92, 0x4f, 0x4f, 0xe3, 0x30,
	0x10, 0xc5, 0xe5, 0x4d, 0xff, 0xce, 0xb6, 0xdb, 0x5d, 0xab, 0x8b, 0x7c, 0x8c, 0x72, 0x40, 0x45,
	0xa0, 0x1e, 0xe0, 0x08, 0xe2, 0xc0, 0x81, 0x1b, 0x07, 0xdc, 0x4f, 0xe0, 0x26, 0x2e, 0x8a, 0x9a,
	0xc4, 0xc1, 0x76, 0x0e, 0xfd, 0x12, 0x7c, 0x66, 0x34, 0x93, 0xa4, 0x09, 0x08, 0x90, 0xe0, 0xf6,
	0xe6, 0xe5, 0xc5, 0xbf, 0x67, 0x6b, 0x60, 0xe6, 0x0e, 0xce, 0xeb, 0x7c, 0x5d, 0x5a, 0xe3, 0x0d,
	0x0f, 0x62, 0x9f, 0x45, 0x1e, 0x66, 0x1b, 0x32, 0x1f, 0x74, 0xbe, 0xd5, 0x96, 0x73, 0x18, 0xa8,
	0x24, 0xb1, 0x82, 0x85, 0x6c, 0x35, 0x95, 0xa4, 0xd1, 0xab, 0xaa, 0x34, 0x11, 0xbf, 0x6a, 0x0f,
	0x35, 0x7a, 0x56, 0x15, 0x7b, 0x11, 0x84, 0x6c, 0x35, 0x97, 0xa4, 0xf9, 0x12, 0x86, 0xce, 0x2b,
	0xaf, 0xc5, 0x80, 0xcc, 0x7a, 0xc0, 0x64, 0x5a, 0xec, 0x8c, 0x18, 0xd6, 0x7f, 0xa3, 0x8e, 0x5e,
	0x18, 0x80, 0x54, 0xc5, 0x5e, 0x6a, 0x57, 0x65, 0xfe, 0x78, 0x18, 0xeb, 0x1d, 0x76, 0x02, 0x23,
	0x15, 0xfb, 0xd4, 0x14, 0x0d, 0xb6, 0x99, 0xb8, 0x80, 0xb1, 0xb6, 0xd6, 0x58, 0x9d, 0x10, 0x7b,
	0x22, 0xdb, 0x91, 0xff, 0x85, 0x20, 0x77, 0x4f, 0x04, 0x9f, 0x4a, 0x94, 0x5d, 0xa1, 0xe1, 0xbb,
	0x42, 0x74, 0xc5, 0x51, 0x77, 0xc5, 0x28, 0x86, 0x79, 0xfd, 0x0c, 0x1b, 0x6f, 0x4a, 0xa9, 0x9f,
	0x31, 0x54, 0x5a, 0x5d, 0x52, 0xa5, 0x89, 0x24, 0x8d, 0xde, 0x3e, 0xcd, 0x32, 0x2a, 0x34, 0x91,
	0xa4, 0x11, 0xb1, 0x33, 0x36, 0xd6, 0x4d, 0x99, 0x7a, 0x40, 0x17, 0x2f, 0xe1, 0xc4, 0x20, 0x0c,
	0x10, 0x4c, 0x43, 0x74, 0x0d, 0x7f, 0xfa, 0x10, 0x57, 0xf2, 0x33, 0x18, 0x5b, 0x7a, 0x02, 0x27,
	0x58, 0x18, 0xac, 0x7e, 0x5f, 0x2e, 0xd6, 0xb1, 0xcf, 0xd6, 0xdd, 0xd3, 0xc8, 0xf6, 0x7b, 0x74,
	0x01, 0xcb, 0xfa, 0x67, 0xa9, 0x9d, 0xf6, 0xf7, 0xc6, 0xe6, 0xca, 0x63, 0xd1, 0x23, 0x8a, 0xf5,
	0x51, 0x77, 0xf0, 0xff, 0x83, 0xf4, 0xf7, 0x88, 0xa7, 0x5d, 0x5d, 0x65, 0xbf, 0x60, 0xdd, 0xc0,
	0xe2, 0x4d, 0xee, 0x87, 0x94, 0xc7, 0x4a, 0xdb, 0xc3, 0xe7, 0x94, 0xdb, 0x96, 0xd2, 0xe4, 0x5c,
	0xc9, 0xcf, 0x61, 0x9c, 0xd3, 0xd6, 0xb6, 0x94, 0x7f, 0x44, 0xe9, 0xef, 0xb3, 0x6c, 0x13, 0xdb,
	0x11, 0x2d, 0xfd, 0xd5, 0x6b, 0x00, 0x00, 0x00, 0xff, 0xff, 0x30, 0xf6, 0x13, 0x4e, 0x04, 0x03,
	0x00, 0x00,
}
