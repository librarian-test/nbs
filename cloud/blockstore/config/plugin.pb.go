// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.26.0
// 	protoc        v3.19.0
// source: cloud/blockstore/config/plugin.proto

package config

import (
	protos "github.com/ydb-platform/nbs/cloud/blockstore/public/api/protos"
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

type TPluginConfig struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// VM description
	ClientProfile *protos.TClientProfile `protobuf:"bytes,1,opt,name=ClientProfile" json:"ClientProfile,omitempty"`
	// Explicit performance profile
	ClientPerformanceProfile *protos.TClientPerformanceProfile `protobuf:"bytes,2,opt,name=ClientPerformanceProfile" json:"ClientPerformanceProfile,omitempty"`
	// Path to the file with TClientAppConfig in textual format
	// Legacy: my be a path to the file with TClientConfig
	ClientConfig *string `protobuf:"bytes,3,opt,name=ClientConfig" json:"ClientConfig,omitempty"`
	// Client identifier.
	ClientId *string `protobuf:"bytes,4,opt,name=ClientId" json:"ClientId,omitempty"`
}

func (x *TPluginConfig) Reset() {
	*x = TPluginConfig{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_config_plugin_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TPluginConfig) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TPluginConfig) ProtoMessage() {}

func (x *TPluginConfig) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_config_plugin_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TPluginConfig.ProtoReflect.Descriptor instead.
func (*TPluginConfig) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_config_plugin_proto_rawDescGZIP(), []int{0}
}

func (x *TPluginConfig) GetClientProfile() *protos.TClientProfile {
	if x != nil {
		return x.ClientProfile
	}
	return nil
}

func (x *TPluginConfig) GetClientPerformanceProfile() *protos.TClientPerformanceProfile {
	if x != nil {
		return x.ClientPerformanceProfile
	}
	return nil
}

func (x *TPluginConfig) GetClientConfig() string {
	if x != nil && x.ClientConfig != nil {
		return *x.ClientConfig
	}
	return ""
}

func (x *TPluginConfig) GetClientId() string {
	if x != nil && x.ClientId != nil {
		return *x.ClientId
	}
	return ""
}

type TPluginMountConfig struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Label of volume to mount.
	DiskId *string `protobuf:"bytes,1,opt,name=DiskId" json:"DiskId,omitempty"`
	// Device unix-socket path.
	UnixSocketPath *string `protobuf:"bytes,2,opt,name=UnixSocketPath" json:"UnixSocketPath,omitempty"`
	// Device encryption spec.
	EncryptionSpec *protos.TEncryptionSpec `protobuf:"bytes,3,opt,name=EncryptionSpec" json:"EncryptionSpec,omitempty"`
}

func (x *TPluginMountConfig) Reset() {
	*x = TPluginMountConfig{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_config_plugin_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TPluginMountConfig) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TPluginMountConfig) ProtoMessage() {}

func (x *TPluginMountConfig) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_config_plugin_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TPluginMountConfig.ProtoReflect.Descriptor instead.
func (*TPluginMountConfig) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_config_plugin_proto_rawDescGZIP(), []int{1}
}

func (x *TPluginMountConfig) GetDiskId() string {
	if x != nil && x.DiskId != nil {
		return *x.DiskId
	}
	return ""
}

func (x *TPluginMountConfig) GetUnixSocketPath() string {
	if x != nil && x.UnixSocketPath != nil {
		return *x.UnixSocketPath
	}
	return ""
}

func (x *TPluginMountConfig) GetEncryptionSpec() *protos.TEncryptionSpec {
	if x != nil {
		return x.EncryptionSpec
	}
	return nil
}

var File_cloud_blockstore_config_plugin_proto protoreflect.FileDescriptor

var file_cloud_blockstore_config_plugin_proto_rawDesc = []byte{
	0x0a, 0x24, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f,
	0x72, 0x65, 0x2f, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x2f, 0x70, 0x6c, 0x75, 0x67, 0x69, 0x6e,
	0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x19, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e,
	0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74,
	0x6f, 0x1a, 0x2f, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74,
	0x6f, 0x72, 0x65, 0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f, 0x70,
	0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x63, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x2e, 0x70, 0x72, 0x6f,
	0x74, 0x6f, 0x1a, 0x33, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73,
	0x74, 0x6f, 0x72, 0x65, 0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x65, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f,
	0x6e, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0x92, 0x02, 0x0a, 0x0d, 0x54, 0x50, 0x6c, 0x75,
	0x67, 0x69, 0x6e, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x12, 0x4f, 0x0a, 0x0d, 0x43, 0x6c, 0x69,
	0x65, 0x6e, 0x74, 0x50, 0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b,
	0x32, 0x29, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b,
	0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x54, 0x43, 0x6c,
	0x69, 0x65, 0x6e, 0x74, 0x50, 0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65, 0x52, 0x0d, 0x43, 0x6c, 0x69,
	0x65, 0x6e, 0x74, 0x50, 0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65, 0x12, 0x70, 0x0a, 0x18, 0x43, 0x6c,
	0x69, 0x65, 0x6e, 0x74, 0x50, 0x65, 0x72, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x6e, 0x63, 0x65, 0x50,
	0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x34, 0x2e, 0x4e,
	0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72,
	0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x54, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74,
	0x50, 0x65, 0x72, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x6e, 0x63, 0x65, 0x50, 0x72, 0x6f, 0x66, 0x69,
	0x6c, 0x65, 0x52, 0x18, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x50, 0x65, 0x72, 0x66, 0x6f, 0x72,
	0x6d, 0x61, 0x6e, 0x63, 0x65, 0x50, 0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65, 0x12, 0x22, 0x0a, 0x0c,
	0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x18, 0x03, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x0c, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67,
	0x12, 0x1a, 0x0a, 0x08, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x49, 0x64, 0x18, 0x04, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x08, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x49, 0x64, 0x22, 0xa8, 0x01, 0x0a,
	0x12, 0x54, 0x50, 0x6c, 0x75, 0x67, 0x69, 0x6e, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x43, 0x6f, 0x6e,
	0x66, 0x69, 0x67, 0x12, 0x16, 0x0a, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x18, 0x01, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x12, 0x26, 0x0a, 0x0e, 0x55,
	0x6e, 0x69, 0x78, 0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x50, 0x61, 0x74, 0x68, 0x18, 0x02, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x0e, 0x55, 0x6e, 0x69, 0x78, 0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x50,
	0x61, 0x74, 0x68, 0x12, 0x52, 0x0a, 0x0e, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f,
	0x6e, 0x53, 0x70, 0x65, 0x63, 0x18, 0x03, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2a, 0x2e, 0x4e, 0x43,
	0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65,
	0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x54, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74,
	0x69, 0x6f, 0x6e, 0x53, 0x70, 0x65, 0x63, 0x52, 0x0e, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74,
	0x69, 0x6f, 0x6e, 0x53, 0x70, 0x65, 0x63, 0x42, 0x2a, 0x5a, 0x28, 0x61, 0x2e, 0x79, 0x61, 0x6e,
	0x64, 0x65, 0x78, 0x2d, 0x74, 0x65, 0x61, 0x6d, 0x2e, 0x72, 0x75, 0x2f, 0x63, 0x6c, 0x6f, 0x75,
	0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f, 0x72, 0x65, 0x2f, 0x63, 0x6f, 0x6e,
	0x66, 0x69, 0x67,
}

var (
	file_cloud_blockstore_config_plugin_proto_rawDescOnce sync.Once
	file_cloud_blockstore_config_plugin_proto_rawDescData = file_cloud_blockstore_config_plugin_proto_rawDesc
)

func file_cloud_blockstore_config_plugin_proto_rawDescGZIP() []byte {
	file_cloud_blockstore_config_plugin_proto_rawDescOnce.Do(func() {
		file_cloud_blockstore_config_plugin_proto_rawDescData = protoimpl.X.CompressGZIP(file_cloud_blockstore_config_plugin_proto_rawDescData)
	})
	return file_cloud_blockstore_config_plugin_proto_rawDescData
}

var file_cloud_blockstore_config_plugin_proto_msgTypes = make([]protoimpl.MessageInfo, 2)
var file_cloud_blockstore_config_plugin_proto_goTypes = []interface{}{
	(*TPluginConfig)(nil),                    // 0: NCloud.NBlockStore.NProto.TPluginConfig
	(*TPluginMountConfig)(nil),               // 1: NCloud.NBlockStore.NProto.TPluginMountConfig
	(*protos.TClientProfile)(nil),            // 2: NCloud.NBlockStore.NProto.TClientProfile
	(*protos.TClientPerformanceProfile)(nil), // 3: NCloud.NBlockStore.NProto.TClientPerformanceProfile
	(*protos.TEncryptionSpec)(nil),           // 4: NCloud.NBlockStore.NProto.TEncryptionSpec
}
var file_cloud_blockstore_config_plugin_proto_depIdxs = []int32{
	2, // 0: NCloud.NBlockStore.NProto.TPluginConfig.ClientProfile:type_name -> NCloud.NBlockStore.NProto.TClientProfile
	3, // 1: NCloud.NBlockStore.NProto.TPluginConfig.ClientPerformanceProfile:type_name -> NCloud.NBlockStore.NProto.TClientPerformanceProfile
	4, // 2: NCloud.NBlockStore.NProto.TPluginMountConfig.EncryptionSpec:type_name -> NCloud.NBlockStore.NProto.TEncryptionSpec
	3, // [3:3] is the sub-list for method output_type
	3, // [3:3] is the sub-list for method input_type
	3, // [3:3] is the sub-list for extension type_name
	3, // [3:3] is the sub-list for extension extendee
	0, // [0:3] is the sub-list for field type_name
}

func init() { file_cloud_blockstore_config_plugin_proto_init() }
func file_cloud_blockstore_config_plugin_proto_init() {
	if File_cloud_blockstore_config_plugin_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_cloud_blockstore_config_plugin_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TPluginConfig); i {
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
		file_cloud_blockstore_config_plugin_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TPluginMountConfig); i {
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
			RawDescriptor: file_cloud_blockstore_config_plugin_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   2,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_cloud_blockstore_config_plugin_proto_goTypes,
		DependencyIndexes: file_cloud_blockstore_config_plugin_proto_depIdxs,
		MessageInfos:      file_cloud_blockstore_config_plugin_proto_msgTypes,
	}.Build()
	File_cloud_blockstore_config_plugin_proto = out.File
	file_cloud_blockstore_config_plugin_proto_rawDesc = nil
	file_cloud_blockstore_config_plugin_proto_goTypes = nil
	file_cloud_blockstore_config_plugin_proto_depIdxs = nil
}
