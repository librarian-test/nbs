// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.26.0
// 	protoc        v3.19.0
// source: cloud/storage/core/tools/common/go/configurator/kikimr-proto/auth.proto

package kikimr_proto

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type TAuthConfig struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	AccessServiceEndpoint      *string `protobuf:"bytes,1,opt,name=AccessServiceEndpoint" json:"AccessServiceEndpoint,omitempty"`
	UserAccountServiceEndpoint *string `protobuf:"bytes,2,opt,name=UserAccountServiceEndpoint" json:"UserAccountServiceEndpoint,omitempty"`
	UseBlackBox                *bool   `protobuf:"varint,3,opt,name=UseBlackBox" json:"UseBlackBox,omitempty"`
	UseAccessService           *bool   `protobuf:"varint,4,opt,name=UseAccessService" json:"UseAccessService,omitempty"`
	UseStaff                   *bool   `protobuf:"varint,5,opt,name=UseStaff" json:"UseStaff,omitempty"`
	UseUserAccountService      *bool   `protobuf:"varint,6,opt,name=UseUserAccountService" json:"UseUserAccountService,omitempty"`
	PathToRootCA               *string `protobuf:"bytes,7,opt,name=PathToRootCA" json:"PathToRootCA,omitempty"`
}

func (x *TAuthConfig) Reset() {
	*x = TAuthConfig{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TAuthConfig) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TAuthConfig) ProtoMessage() {}

func (x *TAuthConfig) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TAuthConfig.ProtoReflect.Descriptor instead.
func (*TAuthConfig) Descriptor() ([]byte, []int) {
	return file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescGZIP(), []int{0}
}

func (x *TAuthConfig) GetAccessServiceEndpoint() string {
	if x != nil && x.AccessServiceEndpoint != nil {
		return *x.AccessServiceEndpoint
	}
	return ""
}

func (x *TAuthConfig) GetUserAccountServiceEndpoint() string {
	if x != nil && x.UserAccountServiceEndpoint != nil {
		return *x.UserAccountServiceEndpoint
	}
	return ""
}

func (x *TAuthConfig) GetUseBlackBox() bool {
	if x != nil && x.UseBlackBox != nil {
		return *x.UseBlackBox
	}
	return false
}

func (x *TAuthConfig) GetUseAccessService() bool {
	if x != nil && x.UseAccessService != nil {
		return *x.UseAccessService
	}
	return false
}

func (x *TAuthConfig) GetUseStaff() bool {
	if x != nil && x.UseStaff != nil {
		return *x.UseStaff
	}
	return false
}

func (x *TAuthConfig) GetUseUserAccountService() bool {
	if x != nil && x.UseUserAccountService != nil {
		return *x.UseUserAccountService
	}
	return false
}

func (x *TAuthConfig) GetPathToRootCA() string {
	if x != nil && x.PathToRootCA != nil {
		return *x.PathToRootCA
	}
	return ""
}

var File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto protoreflect.FileDescriptor

var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDesc = []byte{
	0x0a, 0x47, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x73, 0x74, 0x6f, 0x72, 0x61, 0x67, 0x65, 0x2f,
	0x63, 0x6f, 0x72, 0x65, 0x2f, 0x74, 0x6f, 0x6f, 0x6c, 0x73, 0x2f, 0x63, 0x6f, 0x6d, 0x6d, 0x6f,
	0x6e, 0x2f, 0x67, 0x6f, 0x2f, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x6f,
	0x72, 0x2f, 0x6b, 0x69, 0x6b, 0x69, 0x6d, 0x72, 0x2d, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x61,
	0x75, 0x74, 0x68, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0xc7, 0x02, 0x0a, 0x0b, 0x54, 0x41,
	0x75, 0x74, 0x68, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x12, 0x34, 0x0a, 0x15, 0x41, 0x63, 0x63,
	0x65, 0x73, 0x73, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x45, 0x6e, 0x64, 0x70, 0x6f, 0x69,
	0x6e, 0x74, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x15, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73,
	0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x45, 0x6e, 0x64, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x12,
	0x3e, 0x0a, 0x1a, 0x55, 0x73, 0x65, 0x72, 0x41, 0x63, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x53, 0x65,
	0x72, 0x76, 0x69, 0x63, 0x65, 0x45, 0x6e, 0x64, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x18, 0x02, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x1a, 0x55, 0x73, 0x65, 0x72, 0x41, 0x63, 0x63, 0x6f, 0x75, 0x6e, 0x74,
	0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x45, 0x6e, 0x64, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x12,
	0x20, 0x0a, 0x0b, 0x55, 0x73, 0x65, 0x42, 0x6c, 0x61, 0x63, 0x6b, 0x42, 0x6f, 0x78, 0x18, 0x03,
	0x20, 0x01, 0x28, 0x08, 0x52, 0x0b, 0x55, 0x73, 0x65, 0x42, 0x6c, 0x61, 0x63, 0x6b, 0x42, 0x6f,
	0x78, 0x12, 0x2a, 0x0a, 0x10, 0x55, 0x73, 0x65, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x53, 0x65,
	0x72, 0x76, 0x69, 0x63, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x08, 0x52, 0x10, 0x55, 0x73, 0x65,
	0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x12, 0x1a, 0x0a,
	0x08, 0x55, 0x73, 0x65, 0x53, 0x74, 0x61, 0x66, 0x66, 0x18, 0x05, 0x20, 0x01, 0x28, 0x08, 0x52,
	0x08, 0x55, 0x73, 0x65, 0x53, 0x74, 0x61, 0x66, 0x66, 0x12, 0x34, 0x0a, 0x15, 0x55, 0x73, 0x65,
	0x55, 0x73, 0x65, 0x72, 0x41, 0x63, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x53, 0x65, 0x72, 0x76, 0x69,
	0x63, 0x65, 0x18, 0x06, 0x20, 0x01, 0x28, 0x08, 0x52, 0x15, 0x55, 0x73, 0x65, 0x55, 0x73, 0x65,
	0x72, 0x41, 0x63, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x12,
	0x22, 0x0a, 0x0c, 0x50, 0x61, 0x74, 0x68, 0x54, 0x6f, 0x52, 0x6f, 0x6f, 0x74, 0x43, 0x41, 0x18,
	0x07, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0c, 0x50, 0x61, 0x74, 0x68, 0x54, 0x6f, 0x52, 0x6f, 0x6f,
	0x74, 0x43, 0x41, 0x42, 0x4f, 0x5a, 0x4d, 0x61, 0x2e, 0x79, 0x61, 0x6e, 0x64, 0x65, 0x78, 0x2d,
	0x74, 0x65, 0x61, 0x6d, 0x2e, 0x72, 0x75, 0x2f, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x73, 0x74,
	0x6f, 0x72, 0x61, 0x67, 0x65, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f, 0x74, 0x6f, 0x6f, 0x6c, 0x73,
	0x2f, 0x63, 0x6f, 0x6d, 0x6d, 0x6f, 0x6e, 0x2f, 0x67, 0x6f, 0x2f, 0x63, 0x6f, 0x6e, 0x66, 0x69,
	0x67, 0x75, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x2f, 0x6b, 0x69, 0x6b, 0x69, 0x6d, 0x72, 0x2d, 0x70,
	0x72, 0x6f, 0x74, 0x6f,
}

var (
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescOnce sync.Once
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescData = file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDesc
)

func file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescGZIP() []byte {
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescOnce.Do(func() {
		file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescData = protoimpl.X.CompressGZIP(file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescData)
	})
	return file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDescData
}

var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_msgTypes = make([]protoimpl.MessageInfo, 1)
var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_goTypes = []interface{}{
	(*TAuthConfig)(nil), // 0: TAuthConfig
}
var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_depIdxs = []int32{
	0, // [0:0] is the sub-list for method output_type
	0, // [0:0] is the sub-list for method input_type
	0, // [0:0] is the sub-list for extension type_name
	0, // [0:0] is the sub-list for extension extendee
	0, // [0:0] is the sub-list for field type_name
}

func init() { file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_init() }
func file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_init() {
	if File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TAuthConfig); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   1,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_goTypes,
		DependencyIndexes: file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_depIdxs,
		MessageInfos:      file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_msgTypes,
	}.Build()
	File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto = out.File
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_rawDesc = nil
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_goTypes = nil
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_auth_proto_depIdxs = nil
}
