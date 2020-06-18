// Code generated by protoc-gen-go. DO NOT EDIT.
// source: control.proto

package ctl

import (
	context "context"
	fmt "fmt"
	proto "github.com/golang/protobuf/proto"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
	math "math"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion3 // please upgrade the proto package

func init() {
	proto.RegisterFile("control.proto", fileDescriptor_0c5120591600887d)
}

var fileDescriptor_0c5120591600887d = []byte{
	// 281 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x74, 0xd2, 0xcd, 0x4a, 0xc3, 0x40,
	0x10, 0x07, 0x70, 0x41, 0x51, 0x58, 0x4d, 0xc1, 0xa9, 0x56, 0xcc, 0xd1, 0x07, 0xc8, 0x41, 0x0f,
	0x82, 0x27, 0xa1, 0xd0, 0x93, 0x8a, 0x36, 0x4f, 0xb0, 0x86, 0xb1, 0x88, 0x49, 0x36, 0xce, 0x8e,
	0x94, 0x3e, 0xa2, 0x6f, 0x25, 0xfb, 0x11, 0x33, 0xdb, 0xa6, 0xc7, 0xfd, 0x75, 0xfe, 0x3b, 0x9d,
	0xc9, 0xaa, 0xac, 0x32, 0x2d, 0x93, 0xa9, 0x8b, 0x8e, 0x0c, 0x1b, 0x38, 0xac, 0xb8, 0xce, 0x33,
	0xcb, 0x86, 0xf4, 0x0a, 0x83, 0xe5, 0x59, 0x8b, 0xbc, 0x36, 0xf4, 0x15, 0x8f, 0x67, 0x76, 0x63,
	0x19, 0x9b, 0x78, 0x9a, 0x7c, 0x7c, 0x52, 0xb3, 0xd6, 0x14, 0x8b, 0x6f, 0x7f, 0x8f, 0xd4, 0xc9,
	0xf3, 0xaa, 0xe1, 0x39, 0xd7, 0x30, 0x57, 0x93, 0x32, 0xdc, 0xf4, 0x4a, 0xd8, 0x69, 0x42, 0x98,
	0x15, 0x15, 0xd7, 0x45, 0x8a, 0x4b, 0xfc, 0xce, 0xaf, 0x46, 0xdd, 0x76, 0x37, 0x07, 0xf0, 0xa0,
	0x4e, 0xa3, 0x97, 0x95, 0x6e, 0x61, 0x2a, 0x2b, 0x9d, 0xb8, 0xf8, 0xc5, 0x2e, 0xfa, 0xec, 0xa3,
	0xca, 0x22, 0x2e, 0x0c, 0x35, 0x9a, 0xe1, 0x52, 0x16, 0x06, 0x73, 0xf9, 0xd9, 0x18, 0xff, 0x77,
	0xf7, 0xe3, 0xbe, 0xfd, 0x20, 0x6d, 0xfa, 0xee, 0x83, 0x88, 0xee, 0x12, 0x7d, 0xf6, 0x5e, 0xa9,
	0x80, 0x25, 0x9b, 0x0e, 0x40, 0x54, 0x39, 0x70, 0xc9, 0xe9, 0x8e, 0xf9, 0xe0, 0x93, 0x3a, 0x0f,
	0xb6, 0x44, 0x8b, 0x1c, 0xff, 0xfa, 0xb5, 0xa8, 0x15, 0xee, 0xae, 0xc9, 0xf7, 0xfd, 0x94, 0x8e,
	0x50, 0xb2, 0x26, 0x86, 0xb4, 0xa7, 0x26, 0xde, 0x1e, 0x21, 0x62, 0x9f, 0x7d, 0x09, 0x1f, 0x5f,
	0x2c, 0x5f, 0xc8, 0x90, 0x4d, 0xb0, 0x5f, 0xfe, 0x22, 0xbe, 0x8d, 0xb0, 0xbc, 0xb0, 0xfc, 0xc4,
	0x86, 0xe5, 0x6f, 0xb1, 0xbb, 0xe1, 0xfd, 0xd8, 0x3f, 0xa9, 0xbb, 0xbf, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xe2, 0x46, 0x50, 0x90, 0xa4, 0x02, 0x00, 0x00,
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConnInterface

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion6

// MgmtCtlClient is the client API for MgmtCtl service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
type MgmtCtlClient interface {
	// Prepare nonvolatile storage devices for use with DAOS
	StoragePrepare(ctx context.Context, in *StoragePrepareReq, opts ...grpc.CallOption) (*StoragePrepareResp, error)
	// Retrieve details of nonvolatile storage on server, including health info
	StorageScan(ctx context.Context, in *StorageScanReq, opts ...grpc.CallOption) (*StorageScanResp, error)
	// Format nonvolatile storage devices for use with DAOS
	StorageFormat(ctx context.Context, in *StorageFormatReq, opts ...grpc.CallOption) (*StorageFormatResp, error)
	// Query DAOS system status
	SystemQuery(ctx context.Context, in *SystemQueryReq, opts ...grpc.CallOption) (*SystemQueryResp, error)
	// Stop DAOS system (shutdown data-plane instances)
	SystemStop(ctx context.Context, in *SystemStopReq, opts ...grpc.CallOption) (*SystemStopResp, error)
	// ResetFormat DAOS system (restart data-plane instances)
	SystemResetFormat(ctx context.Context, in *SystemResetFormatReq, opts ...grpc.CallOption) (*SystemResetFormatResp, error)
	// Start DAOS system (restart data-plane instances)
	SystemStart(ctx context.Context, in *SystemStartReq, opts ...grpc.CallOption) (*SystemStartResp, error)
	// Perform a fabric scan to determine the available provider, device, NUMA node combinations
	NetworkScan(ctx context.Context, in *NetworkScanReq, opts ...grpc.CallOption) (*NetworkScanResp, error)
	// Retrieve firmware details from storage devices on server
	FirmwareQuery(ctx context.Context, in *FirmwareQueryReq, opts ...grpc.CallOption) (*FirmwareQueryResp, error)
}

type mgmtCtlClient struct {
	cc grpc.ClientConnInterface
}

func NewMgmtCtlClient(cc grpc.ClientConnInterface) MgmtCtlClient {
	return &mgmtCtlClient{cc}
}

func (c *mgmtCtlClient) StoragePrepare(ctx context.Context, in *StoragePrepareReq, opts ...grpc.CallOption) (*StoragePrepareResp, error) {
	out := new(StoragePrepareResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/StoragePrepare", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) StorageScan(ctx context.Context, in *StorageScanReq, opts ...grpc.CallOption) (*StorageScanResp, error) {
	out := new(StorageScanResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/StorageScan", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) StorageFormat(ctx context.Context, in *StorageFormatReq, opts ...grpc.CallOption) (*StorageFormatResp, error) {
	out := new(StorageFormatResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/StorageFormat", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) SystemQuery(ctx context.Context, in *SystemQueryReq, opts ...grpc.CallOption) (*SystemQueryResp, error) {
	out := new(SystemQueryResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/SystemQuery", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) SystemStop(ctx context.Context, in *SystemStopReq, opts ...grpc.CallOption) (*SystemStopResp, error) {
	out := new(SystemStopResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/SystemStop", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) SystemResetFormat(ctx context.Context, in *SystemResetFormatReq, opts ...grpc.CallOption) (*SystemResetFormatResp, error) {
	out := new(SystemResetFormatResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/SystemResetFormat", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) SystemStart(ctx context.Context, in *SystemStartReq, opts ...grpc.CallOption) (*SystemStartResp, error) {
	out := new(SystemStartResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/SystemStart", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) NetworkScan(ctx context.Context, in *NetworkScanReq, opts ...grpc.CallOption) (*NetworkScanResp, error) {
	out := new(NetworkScanResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/NetworkScan", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *mgmtCtlClient) FirmwareQuery(ctx context.Context, in *FirmwareQueryReq, opts ...grpc.CallOption) (*FirmwareQueryResp, error) {
	out := new(FirmwareQueryResp)
	err := c.cc.Invoke(ctx, "/ctl.MgmtCtl/FirmwareQuery", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// MgmtCtlServer is the server API for MgmtCtl service.
type MgmtCtlServer interface {
	// Prepare nonvolatile storage devices for use with DAOS
	StoragePrepare(context.Context, *StoragePrepareReq) (*StoragePrepareResp, error)
	// Retrieve details of nonvolatile storage on server, including health info
	StorageScan(context.Context, *StorageScanReq) (*StorageScanResp, error)
	// Format nonvolatile storage devices for use with DAOS
	StorageFormat(context.Context, *StorageFormatReq) (*StorageFormatResp, error)
	// Query DAOS system status
	SystemQuery(context.Context, *SystemQueryReq) (*SystemQueryResp, error)
	// Stop DAOS system (shutdown data-plane instances)
	SystemStop(context.Context, *SystemStopReq) (*SystemStopResp, error)
	// ResetFormat DAOS system (restart data-plane instances)
	SystemResetFormat(context.Context, *SystemResetFormatReq) (*SystemResetFormatResp, error)
	// Start DAOS system (restart data-plane instances)
	SystemStart(context.Context, *SystemStartReq) (*SystemStartResp, error)
	// Perform a fabric scan to determine the available provider, device, NUMA node combinations
	NetworkScan(context.Context, *NetworkScanReq) (*NetworkScanResp, error)
	// Retrieve firmware details from storage devices on server
	FirmwareQuery(context.Context, *FirmwareQueryReq) (*FirmwareQueryResp, error)
}

// UnimplementedMgmtCtlServer can be embedded to have forward compatible implementations.
type UnimplementedMgmtCtlServer struct {
}

func (*UnimplementedMgmtCtlServer) StoragePrepare(ctx context.Context, req *StoragePrepareReq) (*StoragePrepareResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method StoragePrepare not implemented")
}
func (*UnimplementedMgmtCtlServer) StorageScan(ctx context.Context, req *StorageScanReq) (*StorageScanResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method StorageScan not implemented")
}
func (*UnimplementedMgmtCtlServer) StorageFormat(ctx context.Context, req *StorageFormatReq) (*StorageFormatResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method StorageFormat not implemented")
}
func (*UnimplementedMgmtCtlServer) SystemQuery(ctx context.Context, req *SystemQueryReq) (*SystemQueryResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method SystemQuery not implemented")
}
func (*UnimplementedMgmtCtlServer) SystemStop(ctx context.Context, req *SystemStopReq) (*SystemStopResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method SystemStop not implemented")
}
func (*UnimplementedMgmtCtlServer) SystemResetFormat(ctx context.Context, req *SystemResetFormatReq) (*SystemResetFormatResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method SystemResetFormat not implemented")
}
func (*UnimplementedMgmtCtlServer) SystemStart(ctx context.Context, req *SystemStartReq) (*SystemStartResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method SystemStart not implemented")
}
func (*UnimplementedMgmtCtlServer) NetworkScan(ctx context.Context, req *NetworkScanReq) (*NetworkScanResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method NetworkScan not implemented")
}
func (*UnimplementedMgmtCtlServer) FirmwareQuery(ctx context.Context, req *FirmwareQueryReq) (*FirmwareQueryResp, error) {
	return nil, status.Errorf(codes.Unimplemented, "method FirmwareQuery not implemented")
}

func RegisterMgmtCtlServer(s *grpc.Server, srv MgmtCtlServer) {
	s.RegisterService(&_MgmtCtl_serviceDesc, srv)
}

func _MgmtCtl_StoragePrepare_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(StoragePrepareReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).StoragePrepare(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/StoragePrepare",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).StoragePrepare(ctx, req.(*StoragePrepareReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_StorageScan_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(StorageScanReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).StorageScan(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/StorageScan",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).StorageScan(ctx, req.(*StorageScanReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_StorageFormat_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(StorageFormatReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).StorageFormat(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/StorageFormat",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).StorageFormat(ctx, req.(*StorageFormatReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_SystemQuery_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SystemQueryReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).SystemQuery(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/SystemQuery",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).SystemQuery(ctx, req.(*SystemQueryReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_SystemStop_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SystemStopReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).SystemStop(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/SystemStop",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).SystemStop(ctx, req.(*SystemStopReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_SystemResetFormat_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SystemResetFormatReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).SystemResetFormat(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/SystemResetFormat",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).SystemResetFormat(ctx, req.(*SystemResetFormatReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_SystemStart_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SystemStartReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).SystemStart(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/SystemStart",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).SystemStart(ctx, req.(*SystemStartReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_NetworkScan_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(NetworkScanReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).NetworkScan(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/NetworkScan",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).NetworkScan(ctx, req.(*NetworkScanReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _MgmtCtl_FirmwareQuery_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(FirmwareQueryReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MgmtCtlServer).FirmwareQuery(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/ctl.MgmtCtl/FirmwareQuery",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MgmtCtlServer).FirmwareQuery(ctx, req.(*FirmwareQueryReq))
	}
	return interceptor(ctx, in, info, handler)
}

var _MgmtCtl_serviceDesc = grpc.ServiceDesc{
	ServiceName: "ctl.MgmtCtl",
	HandlerType: (*MgmtCtlServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "StoragePrepare",
			Handler:    _MgmtCtl_StoragePrepare_Handler,
		},
		{
			MethodName: "StorageScan",
			Handler:    _MgmtCtl_StorageScan_Handler,
		},
		{
			MethodName: "StorageFormat",
			Handler:    _MgmtCtl_StorageFormat_Handler,
		},
		{
			MethodName: "SystemQuery",
			Handler:    _MgmtCtl_SystemQuery_Handler,
		},
		{
			MethodName: "SystemStop",
			Handler:    _MgmtCtl_SystemStop_Handler,
		},
		{
			MethodName: "SystemResetFormat",
			Handler:    _MgmtCtl_SystemResetFormat_Handler,
		},
		{
			MethodName: "SystemStart",
			Handler:    _MgmtCtl_SystemStart_Handler,
		},
		{
			MethodName: "NetworkScan",
			Handler:    _MgmtCtl_NetworkScan_Handler,
		},
		{
			MethodName: "FirmwareQuery",
			Handler:    _MgmtCtl_FirmwareQuery_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "control.proto",
}
