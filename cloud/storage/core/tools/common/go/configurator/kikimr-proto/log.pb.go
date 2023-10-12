// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.26.0
// 	protoc        v3.19.0
// source: cloud/storage/core/tools/common/go/configurator/kikimr-proto/log.proto

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

type TLogConfig struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Entry           []*TLogConfig_TEntry `protobuf:"bytes,1,rep,name=Entry" json:"Entry,omitempty"`
	SysLog          *bool                `protobuf:"varint,2,opt,name=SysLog" json:"SysLog,omitempty"`
	DefaultLevel    *uint32              `protobuf:"varint,4,opt,name=DefaultLevel" json:"DefaultLevel,omitempty"`
	SysLogService   *string              `protobuf:"bytes,5,opt,name=SysLogService" json:"SysLogService,omitempty"`
	TimeThresholdMs *uint64              `protobuf:"varint,6,opt,name=TimeThresholdMs" json:"TimeThresholdMs,omitempty"`
}

func (x *TLogConfig) Reset() {
	*x = TLogConfig{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TLogConfig) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TLogConfig) ProtoMessage() {}

func (x *TLogConfig) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TLogConfig.ProtoReflect.Descriptor instead.
func (*TLogConfig) Descriptor() ([]byte, []int) {
	return file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescGZIP(), []int{0}
}

func (x *TLogConfig) GetEntry() []*TLogConfig_TEntry {
	if x != nil {
		return x.Entry
	}
	return nil
}

func (x *TLogConfig) GetSysLog() bool {
	if x != nil && x.SysLog != nil {
		return *x.SysLog
	}
	return false
}

func (x *TLogConfig) GetDefaultLevel() uint32 {
	if x != nil && x.DefaultLevel != nil {
		return *x.DefaultLevel
	}
	return 0
}

func (x *TLogConfig) GetSysLogService() string {
	if x != nil && x.SysLogService != nil {
		return *x.SysLogService
	}
	return ""
}

func (x *TLogConfig) GetTimeThresholdMs() uint64 {
	if x != nil && x.TimeThresholdMs != nil {
		return *x.TimeThresholdMs
	}
	return 0
}

type TLogConfig_TEntry struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Component     []byte  `protobuf:"bytes,1,opt,name=Component" json:"Component,omitempty"`
	Level         *uint32 `protobuf:"varint,2,opt,name=Level" json:"Level,omitempty"`
	SamplingLevel *uint32 `protobuf:"varint,3,opt,name=SamplingLevel" json:"SamplingLevel,omitempty"`
	SamplingRate  *uint32 `protobuf:"varint,4,opt,name=SamplingRate" json:"SamplingRate,omitempty"`
}

func (x *TLogConfig_TEntry) Reset() {
	*x = TLogConfig_TEntry{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TLogConfig_TEntry) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TLogConfig_TEntry) ProtoMessage() {}

func (x *TLogConfig_TEntry) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TLogConfig_TEntry.ProtoReflect.Descriptor instead.
func (*TLogConfig_TEntry) Descriptor() ([]byte, []int) {
	return file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescGZIP(), []int{0, 0}
}

func (x *TLogConfig_TEntry) GetComponent() []byte {
	if x != nil {
		return x.Component
	}
	return nil
}

func (x *TLogConfig_TEntry) GetLevel() uint32 {
	if x != nil && x.Level != nil {
		return *x.Level
	}
	return 0
}

func (x *TLogConfig_TEntry) GetSamplingLevel() uint32 {
	if x != nil && x.SamplingLevel != nil {
		return *x.SamplingLevel
	}
	return 0
}

func (x *TLogConfig_TEntry) GetSamplingRate() uint32 {
	if x != nil && x.SamplingRate != nil {
		return *x.SamplingRate
	}
	return 0
}

var File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto protoreflect.FileDescriptor

var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDesc = []byte{
	0x0a, 0x46, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x73, 0x74, 0x6f, 0x72, 0x61, 0x67, 0x65, 0x2f,
	0x63, 0x6f, 0x72, 0x65, 0x2f, 0x74, 0x6f, 0x6f, 0x6c, 0x73, 0x2f, 0x63, 0x6f, 0x6d, 0x6d, 0x6f,
	0x6e, 0x2f, 0x67, 0x6f, 0x2f, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x6f,
	0x72, 0x2f, 0x6b, 0x69, 0x6b, 0x69, 0x6d, 0x72, 0x2d, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x6c,
	0x6f, 0x67, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0xcb, 0x02, 0x0a, 0x0a, 0x54, 0x4c, 0x6f,
	0x67, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x12, 0x28, 0x0a, 0x05, 0x45, 0x6e, 0x74, 0x72, 0x79,
	0x18, 0x01, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x12, 0x2e, 0x54, 0x4c, 0x6f, 0x67, 0x43, 0x6f, 0x6e,
	0x66, 0x69, 0x67, 0x2e, 0x54, 0x45, 0x6e, 0x74, 0x72, 0x79, 0x52, 0x05, 0x45, 0x6e, 0x74, 0x72,
	0x79, 0x12, 0x16, 0x0a, 0x06, 0x53, 0x79, 0x73, 0x4c, 0x6f, 0x67, 0x18, 0x02, 0x20, 0x01, 0x28,
	0x08, 0x52, 0x06, 0x53, 0x79, 0x73, 0x4c, 0x6f, 0x67, 0x12, 0x22, 0x0a, 0x0c, 0x44, 0x65, 0x66,
	0x61, 0x75, 0x6c, 0x74, 0x4c, 0x65, 0x76, 0x65, 0x6c, 0x18, 0x04, 0x20, 0x01, 0x28, 0x0d, 0x52,
	0x0c, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x4c, 0x65, 0x76, 0x65, 0x6c, 0x12, 0x24, 0x0a,
	0x0d, 0x53, 0x79, 0x73, 0x4c, 0x6f, 0x67, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x18, 0x05,
	0x20, 0x01, 0x28, 0x09, 0x52, 0x0d, 0x53, 0x79, 0x73, 0x4c, 0x6f, 0x67, 0x53, 0x65, 0x72, 0x76,
	0x69, 0x63, 0x65, 0x12, 0x28, 0x0a, 0x0f, 0x54, 0x69, 0x6d, 0x65, 0x54, 0x68, 0x72, 0x65, 0x73,
	0x68, 0x6f, 0x6c, 0x64, 0x4d, 0x73, 0x18, 0x06, 0x20, 0x01, 0x28, 0x04, 0x52, 0x0f, 0x54, 0x69,
	0x6d, 0x65, 0x54, 0x68, 0x72, 0x65, 0x73, 0x68, 0x6f, 0x6c, 0x64, 0x4d, 0x73, 0x1a, 0x86, 0x01,
	0x0a, 0x06, 0x54, 0x45, 0x6e, 0x74, 0x72, 0x79, 0x12, 0x1c, 0x0a, 0x09, 0x43, 0x6f, 0x6d, 0x70,
	0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0c, 0x52, 0x09, 0x43, 0x6f, 0x6d,
	0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x12, 0x14, 0x0a, 0x05, 0x4c, 0x65, 0x76, 0x65, 0x6c, 0x18,
	0x02, 0x20, 0x01, 0x28, 0x0d, 0x52, 0x05, 0x4c, 0x65, 0x76, 0x65, 0x6c, 0x12, 0x24, 0x0a, 0x0d,
	0x53, 0x61, 0x6d, 0x70, 0x6c, 0x69, 0x6e, 0x67, 0x4c, 0x65, 0x76, 0x65, 0x6c, 0x18, 0x03, 0x20,
	0x01, 0x28, 0x0d, 0x52, 0x0d, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x69, 0x6e, 0x67, 0x4c, 0x65, 0x76,
	0x65, 0x6c, 0x12, 0x22, 0x0a, 0x0c, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x69, 0x6e, 0x67, 0x52, 0x61,
	0x74, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x0d, 0x52, 0x0c, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x69,
	0x6e, 0x67, 0x52, 0x61, 0x74, 0x65, 0x42, 0x4f, 0x5a, 0x4d, 0x61, 0x2e, 0x79, 0x61, 0x6e, 0x64,
	0x65, 0x78, 0x2d, 0x74, 0x65, 0x61, 0x6d, 0x2e, 0x72, 0x75, 0x2f, 0x63, 0x6c, 0x6f, 0x75, 0x64,
	0x2f, 0x73, 0x74, 0x6f, 0x72, 0x61, 0x67, 0x65, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f, 0x74, 0x6f,
	0x6f, 0x6c, 0x73, 0x2f, 0x63, 0x6f, 0x6d, 0x6d, 0x6f, 0x6e, 0x2f, 0x67, 0x6f, 0x2f, 0x63, 0x6f,
	0x6e, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x2f, 0x6b, 0x69, 0x6b, 0x69, 0x6d,
	0x72, 0x2d, 0x70, 0x72, 0x6f, 0x74, 0x6f,
}

var (
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescOnce sync.Once
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescData = file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDesc
)

func file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescGZIP() []byte {
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescOnce.Do(func() {
		file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescData = protoimpl.X.CompressGZIP(file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescData)
	})
	return file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDescData
}

var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes = make([]protoimpl.MessageInfo, 2)
var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_goTypes = []interface{}{
	(*TLogConfig)(nil),        // 0: TLogConfig
	(*TLogConfig_TEntry)(nil), // 1: TLogConfig.TEntry
}
var file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_depIdxs = []int32{
	1, // 0: TLogConfig.Entry:type_name -> TLogConfig.TEntry
	1, // [1:1] is the sub-list for method output_type
	1, // [1:1] is the sub-list for method input_type
	1, // [1:1] is the sub-list for extension type_name
	1, // [1:1] is the sub-list for extension extendee
	0, // [0:1] is the sub-list for field type_name
}

func init() { file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_init() }
func file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_init() {
	if File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TLogConfig); i {
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
		file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TLogConfig_TEntry); i {
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
			RawDescriptor: file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   2,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_goTypes,
		DependencyIndexes: file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_depIdxs,
		MessageInfos:      file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_msgTypes,
	}.Build()
	File_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto = out.File
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_rawDesc = nil
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_goTypes = nil
	file_cloud_storage_core_tools_common_go_configurator_kikimr_proto_log_proto_depIdxs = nil
}
