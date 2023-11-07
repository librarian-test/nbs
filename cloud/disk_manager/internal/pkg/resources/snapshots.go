package resources

import (
	"bytes"
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/ydb-platform/nbs/cloud/disk_manager/internal/pkg/logging"
	"github.com/ydb-platform/nbs/cloud/disk_manager/internal/pkg/persistence"
	"github.com/ydb-platform/nbs/cloud/disk_manager/internal/pkg/tasks/errors"
	"github.com/ydb-platform/nbs/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type snapshotStatus uint32

func (s *snapshotStatus) UnmarshalYDB(res persistence.RawValue) error {
	*s = snapshotStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	snapshotStatusCreating snapshotStatus = iota
	snapshotStatusReady    snapshotStatus = iota
	snapshotStatusDeleting snapshotStatus = iota
	snapshotStatusDeleted  snapshotStatus = iota
)

func snapshotStatusToString(status snapshotStatus) string {
	switch status {
	case snapshotStatusCreating:
		return "creating"
	case snapshotStatusReady:
		return "ready"
	case snapshotStatusDeleting:
		return "deleting"
	case snapshotStatusDeleted:
		return "deleted"
	}

	return fmt.Sprintf("unknown_%v", status)
}

////////////////////////////////////////////////////////////////////////////////

// This is mapped into a DB row. If you change this struct, make sure to update
// the mapping code.
type snapshotState struct {
	id                string
	folderID          string
	zoneID            string
	diskID            string
	checkpointID      string
	createRequest     []byte
	createTaskID      string
	creatingAt        time.Time
	createdAt         time.Time
	createdBy         string
	deleteTaskID      string
	deletingAt        time.Time
	deletedAt         time.Time
	baseSnapshotID    string
	baseCheckpointID  string
	useDataplaneTasks bool
	size              uint64
	storageSize       uint64
	lockTaskID        string
	encryptionMode    uint32
	encryptionKeyHash []byte

	status snapshotStatus
}

func (s *snapshotState) toSnapshotMeta() *SnapshotMeta {
	// TODO: Snapshot.CreateRequest should be []byte, because we can't unmarshal
	// it here, without knowing particular protobuf message type.

	return &SnapshotMeta{
		ID:       s.id,
		FolderID: s.folderID,
		Disk: &types.Disk{
			ZoneId: s.zoneID,
			DiskId: s.diskID,
		},
		CheckpointID:      s.checkpointID,
		CreateTaskID:      s.createTaskID,
		CreatingAt:        s.creatingAt,
		CreatedBy:         s.createdBy,
		DeleteTaskID:      s.deleteTaskID,
		BaseSnapshotID:    s.baseSnapshotID,
		BaseCheckpointID:  s.baseCheckpointID,
		UseDataplaneTasks: s.useDataplaneTasks,
		Size:              s.size,
		StorageSize:       s.storageSize,
		LockTaskID:        s.lockTaskID,
		Encryption: &types.EncryptionDesc{
			Mode: types.EncryptionMode(s.encryptionMode),
			Key: &types.EncryptionDesc_KeyHash{
				KeyHash: s.encryptionKeyHash,
			},
		},
		Ready: s.status == snapshotStatusReady,
	}
}

func (s *snapshotState) structValue() persistence.Value {
	return persistence.StructValue(
		persistence.StructFieldValue("id", persistence.UTF8Value(s.id)),
		persistence.StructFieldValue("folder_id", persistence.UTF8Value(s.folderID)),
		persistence.StructFieldValue("zone_id", persistence.UTF8Value(s.zoneID)),
		persistence.StructFieldValue("disk_id", persistence.UTF8Value(s.diskID)),
		persistence.StructFieldValue("checkpoint_id", persistence.UTF8Value(s.checkpointID)),
		persistence.StructFieldValue("create_request", persistence.StringValue(s.createRequest)),
		persistence.StructFieldValue("create_task_id", persistence.UTF8Value(s.createTaskID)),
		persistence.StructFieldValue("creating_at", persistence.TimestampValue(s.creatingAt)),
		persistence.StructFieldValue("created_at", persistence.TimestampValue(s.createdAt)),
		persistence.StructFieldValue("created_by", persistence.UTF8Value(s.createdBy)),
		persistence.StructFieldValue("delete_task_id", persistence.UTF8Value(s.deleteTaskID)),
		persistence.StructFieldValue("deleting_at", persistence.TimestampValue(s.deletingAt)),
		persistence.StructFieldValue("deleted_at", persistence.TimestampValue(s.deletedAt)),
		persistence.StructFieldValue("incremental", persistence.BoolValue(true)), // deprecated
		persistence.StructFieldValue("base_snapshot_id", persistence.UTF8Value(s.baseSnapshotID)),
		persistence.StructFieldValue("base_checkpoint_id", persistence.UTF8Value(s.baseCheckpointID)),
		persistence.StructFieldValue("use_dataplane_tasks", persistence.BoolValue(s.useDataplaneTasks)),
		persistence.StructFieldValue("size", persistence.Uint64Value(s.size)),
		persistence.StructFieldValue("storage_size", persistence.Uint64Value(s.storageSize)),
		persistence.StructFieldValue("lock_task_id", persistence.UTF8Value(s.lockTaskID)),
		persistence.StructFieldValue("encryption_mode", persistence.Uint32Value(s.encryptionMode)),
		persistence.StructFieldValue("encryption_keyhash", persistence.StringValue(s.encryptionKeyHash)),
		persistence.StructFieldValue("status", persistence.Int64Value(int64(s.status))),
	)
}

func scanSnapshotState(res persistence.Result) (state snapshotState, err error) {
	err = res.ScanNamed(
		persistence.OptionalWithDefault("id", &state.id),
		persistence.OptionalWithDefault("folder_id", &state.folderID),
		persistence.OptionalWithDefault("zone_id", &state.zoneID),
		persistence.OptionalWithDefault("disk_id", &state.diskID),
		persistence.OptionalWithDefault("checkpoint_id", &state.checkpointID),
		persistence.OptionalWithDefault("create_request", &state.createRequest),
		persistence.OptionalWithDefault("create_task_id", &state.createTaskID),
		persistence.OptionalWithDefault("creating_at", &state.creatingAt),
		persistence.OptionalWithDefault("created_at", &state.createdAt),
		persistence.OptionalWithDefault("created_by", &state.createdBy),
		persistence.OptionalWithDefault("delete_task_id", &state.deleteTaskID),
		persistence.OptionalWithDefault("deleting_at", &state.deletingAt),
		persistence.OptionalWithDefault("deleted_at", &state.deletedAt),
		persistence.OptionalWithDefault("base_snapshot_id", &state.baseSnapshotID),
		persistence.OptionalWithDefault("base_checkpoint_id", &state.baseCheckpointID),
		persistence.OptionalWithDefault("use_dataplane_tasks", &state.useDataplaneTasks),
		persistence.OptionalWithDefault("size", &state.size),
		persistence.OptionalWithDefault("storage_size", &state.storageSize),
		persistence.OptionalWithDefault("lock_task_id", &state.lockTaskID),
		persistence.OptionalWithDefault("encryption_mode", &state.encryptionMode),
		persistence.OptionalWithDefault("encryption_keyhash", &state.encryptionKeyHash),
		persistence.OptionalWithDefault("status", &state.status),
	)
	if err != nil {
		return state, errors.NewNonRetriableErrorf(
			"scanSnapshotStates: failed to parse row: %w",
			err,
		)
	}

	return state, nil
}

func scanSnapshotStates(
	ctx context.Context,
	res persistence.Result,
) ([]snapshotState, error) {

	var states []snapshotState
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			state, err := scanSnapshotState(res)
			if err != nil {
				return nil, err
			}

			states = append(states, state)
		}
	}

	return states, nil
}

func snapshotStateStructTypeString() string {
	return `Struct<
		id: Utf8,
		folder_id: Utf8,
		zone_id: Utf8,
		disk_id: Utf8,
		checkpoint_id: Utf8,
		create_request: String,
		create_task_id: Utf8,
		creating_at: Timestamp,
		created_at: Timestamp,
		created_by: Utf8,
		delete_task_id: Utf8,
		deleting_at: Timestamp,
		deleted_at: Timestamp,
		incremental: Bool, /* deprecated */
		base_snapshot_id: Utf8,
		base_checkpoint_id: Utf8,
		use_dataplane_tasks: Bool,
		size: Uint64,
		storage_size: Uint64,
		lock_task_id: Utf8,
		encryption_mode: Uint32,
		encryption_keyhash: String,
		status: Int64>`
}

func snapshotStateTableDescription() persistence.CreateTableDescription {
	return persistence.NewCreateTableDescription(
		persistence.WithColumn("id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("folder_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("zone_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("disk_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("checkpoint_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("create_request", persistence.Optional(persistence.TypeString)),
		persistence.WithColumn("create_task_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("creating_at", persistence.Optional(persistence.TypeTimestamp)),
		persistence.WithColumn("created_at", persistence.Optional(persistence.TypeTimestamp)),
		persistence.WithColumn("created_by", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("delete_task_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("deleting_at", persistence.Optional(persistence.TypeTimestamp)),
		persistence.WithColumn("deleted_at", persistence.Optional(persistence.TypeTimestamp)),
		persistence.WithColumn("incremental", persistence.Optional(persistence.TypeBool)), // deprecated
		persistence.WithColumn("base_snapshot_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("base_checkpoint_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("use_dataplane_tasks", persistence.Optional(persistence.TypeBool)),
		persistence.WithColumn("size", persistence.Optional(persistence.TypeUint64)),
		persistence.WithColumn("storage_size", persistence.Optional(persistence.TypeUint64)),
		persistence.WithColumn("lock_task_id", persistence.Optional(persistence.TypeUTF8)),
		persistence.WithColumn("encryption_mode", persistence.Optional(persistence.TypeUint32)),
		persistence.WithColumn("encryption_keyhash", persistence.Optional(persistence.TypeString)),
		persistence.WithColumn("status", persistence.Optional(persistence.TypeInt64)),
		persistence.WithPrimaryKeyColumn("id"),
	)
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) snapshotExists(
	ctx context.Context,
	tx *persistence.Transaction,
	snapshotID string,
) (bool, error) {

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select count(*)
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return false, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return false, nil
	}

	var count uint64
	err = res.ScanWithDefaults(&count)
	if err != nil {
		return false, err
	}

	return count != 0, nil
}

func (s *storageYDB) getIncremental(
	ctx context.Context,
	tx *persistence.Transaction,
	disk *types.Disk,
) (snapshotID string, checkpointID string, err error) {

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $zone_id as Utf8;
		declare $disk_id as Utf8;

		select *
		from incremental
		where zone_id = $zone_id and disk_id = $disk_id
	`, s.snapshotsPath),
		persistence.ValueParam("$zone_id", persistence.UTF8Value(disk.ZoneId)),
		persistence.ValueParam("$disk_id", persistence.UTF8Value(disk.DiskId)),
	)
	if err != nil {
		return "", "", err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			err = res.ScanNamed(
				persistence.OptionalWithDefault("snapshot_id", &snapshotID),
				persistence.OptionalWithDefault("checkpoint_id", &checkpointID),
			)
			if err != nil {
				return "", "", errors.NewNonRetriableErrorf(
					"getIncremental: failed to to parse row: %w",
					err,
				)
			}
		}
	}

	return
}

func (s *storageYDB) getSnapshotMeta(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) (*SnapshotMeta, error) {

	res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		return nil, err
	}

	if len(states) != 0 {
		return states[0].toSnapshotMeta(), nil
	} else {
		return nil, nil
	}
}

func (s *storageYDB) createSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshot SnapshotMeta,
) (*SnapshotMeta, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	// HACK: see NBS-974 for details.
	imageExists, err := s.imageExists(ctx, tx, snapshot.ID)
	if err != nil {
		return nil, err
	}

	if imageExists {
		err = tx.Commit(ctx)
		if err != nil {
			return nil, err
		}

		logging.Info(
			ctx,
			"snapshot with id %v can't be created, because image with id %v already exists",
			snapshot.ID,
			snapshot.ID,
		)
		return nil, nil
	}

	createRequest, err := proto.Marshal(snapshot.CreateRequest)
	if err != nil {
		return nil, errors.NewNonRetriableErrorf(
			"failed to marshal create request for snapshot with id %v: %w",
			snapshot.ID,
			err,
		)
	}

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshot.ID)),
	)
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return nil, commitErr
		}

		return nil, err
	}

	if len(states) != 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return nil, err
		}

		state := states[0]

		if state.status >= snapshotStatusDeleting {
			logging.Info(ctx, "can't create already deleting/deleted snapshot with id %v", snapshot.ID)
			return nil, errors.NewSilentNonRetriableErrorf(
				"can't create already deleting/deleted snapshot with id %v",
				snapshot.ID,
			)
		}

		// Check idempotency.
		if bytes.Equal(state.createRequest, createRequest) &&
			state.createTaskID == snapshot.CreateTaskID &&
			state.createdBy == snapshot.CreatedBy {

			return state.toSnapshotMeta(), nil
		}

		logging.Info(
			ctx,
			"snapshot with different params already exists, old=%v, new=%v",
			state,
			snapshot,
		)
		return nil, nil
	}

	baseSnapshotID, baseCheckpointID, err := s.getIncremental(
		ctx,
		tx,
		snapshot.Disk,
	)
	if err != nil {
		return nil, err
	}

	state := snapshotState{
		id:                snapshot.ID,
		folderID:          snapshot.FolderID,
		zoneID:            snapshot.Disk.ZoneId,
		diskID:            snapshot.Disk.DiskId,
		checkpointID:      snapshot.CheckpointID,
		createRequest:     createRequest,
		createTaskID:      snapshot.CreateTaskID,
		creatingAt:        snapshot.CreatingAt,
		createdBy:         snapshot.CreatedBy,
		baseSnapshotID:    baseSnapshotID,
		baseCheckpointID:  baseCheckpointID,
		useDataplaneTasks: snapshot.UseDataplaneTasks,

		status: snapshotStatusCreating,
	}

	if snapshot.Encryption != nil {
		state.encryptionMode = uint32(snapshot.Encryption.Mode)

		switch key := snapshot.Encryption.Key.(type) {
		case *types.EncryptionDesc_KeyHash:
			state.encryptionKeyHash = key.KeyHash
		case nil:
			state.encryptionKeyHash = nil
		default:
			return nil, errors.NewNonRetriableErrorf("unknown key %s", key)
		}
	} else {
		state.encryptionMode = uint32(types.EncryptionMode_NO_ENCRYPTION)
		state.encryptionKeyHash = nil
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return state.toSnapshotMeta(), nil
}

func (s *storageYDB) snapshotCreated(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	createdAt time.Time,
	snapshotSize uint64,
	snapshotStorageSize uint64,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return errors.NewNonRetriableErrorf(
			"snapshot with id %v is not found",
			snapshotID,
		)
	}

	state := states[0]

	if state.status == snapshotStatusReady {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if state.status != snapshotStatusCreating {
		return errors.NewSilentNonRetriableErrorf(
			"snapshot with id %v and status %v can't be created",
			snapshotID,
			snapshotStatusToString(state.status),
		)
	}

	state.status = snapshotStatusReady
	state.createdAt = createdAt
	state.size = snapshotSize
	state.storageSize = snapshotStorageSize

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return err
	}

	if len(state.baseSnapshotID) == 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $zone_id as Utf8;
			declare $disk_id as Utf8;
			declare $snapshot_id as Utf8;
			declare $checkpoint_id as Utf8;

			upsert into incremental (zone_id, disk_id, snapshot_id, checkpoint_id)
			values ($zone_id, $disk_id, $snapshot_id, $checkpoint_id)
		`, s.snapshotsPath),
			persistence.ValueParam("$zone_id", persistence.UTF8Value(state.zoneID)),
			persistence.ValueParam("$disk_id", persistence.UTF8Value(state.diskID)),
			persistence.ValueParam("$snapshot_id", persistence.UTF8Value(snapshotID)),
			persistence.ValueParam("$checkpoint_id", persistence.UTF8Value(state.checkpointID)),
		)
		if err != nil {
			return err
		}
	} else {
		// Remove previous incremental snapshot and insert new one instead.
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $zone_id as Utf8;
			declare $disk_id as Utf8;
			declare $snapshot_id as Utf8;
			declare $checkpoint_id as Utf8;
			declare $base_snapshot_id as Utf8;

			delete from incremental
			where zone_id = $zone_id and disk_id = $disk_id and snapshot_id = $base_snapshot_id;

			upsert into incremental (zone_id, disk_id, snapshot_id, checkpoint_id)
			values ($zone_id, $disk_id, $snapshot_id, $checkpoint_id)
		`, s.snapshotsPath),
			persistence.ValueParam("$zone_id", persistence.UTF8Value(state.zoneID)),
			persistence.ValueParam("$disk_id", persistence.UTF8Value(state.diskID)),
			persistence.ValueParam("$snapshot_id", persistence.UTF8Value(snapshotID)),
			persistence.ValueParam("$checkpoint_id", persistence.UTF8Value(state.checkpointID)),
			persistence.ValueParam("$base_snapshot_id", persistence.UTF8Value(state.baseSnapshotID)),
		)
		if err != nil {
			return err
		}
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) deleteSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	taskID string,
	deletingAt time.Time,
) (*SnapshotMeta, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	// HACK: see NBS-974 for details.
	imageExists, err := s.imageExists(ctx, tx, snapshotID)
	if err != nil {
		return nil, err
	}

	if imageExists {
		err = tx.Commit(ctx)
		if err != nil {
			return nil, err
		}

		logging.Info(
			ctx,
			"snapshot with id %v can't be deleted, because image with id %v already exists",
			snapshotID,
			snapshotID,
		)
		return nil, nil
	}

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return nil, commitErr
		}

		return nil, err
	}

	var state snapshotState

	if len(states) != 0 {
		state = states[0]

		if state.status >= snapshotStatusDeleting {
			// Snapshot already marked as deleting/deleted.

			err = tx.Commit(ctx)
			if err != nil {
				return nil, err
			}

			return state.toSnapshotMeta(), nil
		}

		if len(state.lockTaskID) != 0 && state.lockTaskID != taskID {
			err = tx.Commit(ctx)
			if err != nil {
				return nil, err
			}

			logging.Debug(
				ctx,
				"Snapshot with id %v is locked, can't delete",
				snapshotID,
			)
			// Prevent deletion.
			return nil, errors.NewInterruptExecutionError()
		}
	}

	state.id = snapshotID
	state.status = snapshotStatusDeleting
	state.deleteTaskID = taskID
	state.deletingAt = deletingAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return nil, err
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $zone_id as Utf8;
		declare $disk_id as Utf8;
		declare $snapshot_id as Utf8;

		delete from incremental
		where zone_id = $zone_id and disk_id = $disk_id and snapshot_id = $snapshot_id
	`, s.snapshotsPath),
		persistence.ValueParam("$zone_id", persistence.UTF8Value(state.zoneID)),
		persistence.ValueParam("$disk_id", persistence.UTF8Value(state.diskID)),
		persistence.ValueParam("$snapshot_id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return state.toSnapshotMeta(), nil
}

func (s *storageYDB) snapshotDeleted(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	deletedAt time.Time,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		// It's possible that snapshot is already collected.
		return tx.Commit(ctx)
	}

	state := states[0]

	if state.status == snapshotStatusDeleted {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if state.status != snapshotStatusDeleting {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return errors.NewNonRetriableErrorf(
			"snapshot with id %v and status %v can't be deleted",
			snapshotID,
			snapshotStatusToString(state.status),
		)
	}

	state.status = snapshotStatusDeleted
	state.deletedAt = deletedAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;
		upsert into snapshots

		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return err
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleted_at as Timestamp;
		declare $snapshot_id as Utf8;

		upsert into deleted (deleted_at, snapshot_id)
		values ($deleted_at, $snapshot_id)
	`, s.snapshotsPath),
		persistence.ValueParam("$deleted_at", persistence.TimestampValue(deletedAt)),
		persistence.ValueParam("$snapshot_id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) clearDeletedSnapshots(
	ctx context.Context,
	session *persistence.Session,
	deletedBefore time.Time,
	limit int,
) error {

	res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleted_before as Timestamp;
		declare $limit as Uint64;

		select *
		from deleted
		where deleted_at < $deleted_before
		limit $limit
	`, s.snapshotsPath),
		persistence.ValueParam("$deleted_before", persistence.TimestampValue(deletedBefore)),
		persistence.ValueParam("$limit", persistence.Uint64Value(uint64(limit))),
	)
	if err != nil {
		return err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var (
				deletedAt  time.Time
				snapshotID string
			)
			err = res.ScanNamed(
				persistence.OptionalWithDefault("deleted_at", &deletedAt),
				persistence.OptionalWithDefault("snapshot_id", &snapshotID),
			)
			if err != nil {
				return errors.NewNonRetriableErrorf(
					"clearDeletedSnapshots: failed to parse row: %w",
					err,
				)
			}

			_, err = session.ExecuteRW(ctx, fmt.Sprintf(`
				--!syntax_v1
				pragma TablePathPrefix = "%v";
				declare $deleted_at as Timestamp;
				declare $snapshot_id as Utf8;
				declare $status as Int64;

				delete from snapshots
				where id = $snapshot_id and status = $status;

				delete from deleted
				where deleted_at = $deleted_at and snapshot_id = $snapshot_id
			`, s.snapshotsPath),
				persistence.ValueParam("$deleted_at", persistence.TimestampValue(deletedAt)),
				persistence.ValueParam("$snapshot_id", persistence.UTF8Value(snapshotID)),
				persistence.ValueParam("$status", persistence.Int64Value(int64(snapshotStatusDeleted))),
			)
			if err != nil {
				return err
			}
		}
	}

	return nil
}

func (s *storageYDB) lockSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	lockTaskID string,
) (locked bool, err error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return false, err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return false, err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return false, commitErr
		}

		return false, err
	}

	if len(states) == 0 {
		return false, tx.Commit(ctx)
	}

	state := states[0]
	if state.status >= snapshotStatusDeleting {
		return false, tx.Commit(ctx)
	}

	if len(state.lockTaskID) != 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return false, err
		}

		if state.lockTaskID == lockTaskID {
			// Should be idempotent.
			return true, nil
		}

		// Unlikely situation. Another lock is found.
		return false, errors.NewInterruptExecutionError()
	}

	state.lockTaskID = lockTaskID

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return false, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return false, err
	}

	return true, nil
}

func (s *storageYDB) unlockSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	lockTaskID string,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.snapshotsPath),
		persistence.ValueParam("$id", persistence.UTF8Value(snapshotID)),
	)
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	state := states[0]
	if state.status >= snapshotStatusDeleting {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if len(state.lockTaskID) == 0 {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if state.lockTaskID != lockTaskID {
		// Our lock is not present, so it's a success.
		return tx.Commit(ctx)
	}

	state.lockTaskID = ""

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.snapshotsPath, snapshotStateStructTypeString()),
		persistence.ValueParam("$states", persistence.ListValue(state.structValue())),
	)
	if err != nil {
		return err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return err
	}

	logging.Debug(ctx, "Successfully unlocked snapshot with id %v", snapshotID)
	return nil
}

func (s *storageYDB) listSnapshots(
	ctx context.Context,
	session *persistence.Session,
	folderID string,
	creatingBefore time.Time,
) ([]string, error) {

	return listResources(
		ctx,
		session,
		s.snapshotsPath,
		"snapshots",
		folderID,
		creatingBefore,
	)
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) CreateSnapshot(
	ctx context.Context,
	snapshot SnapshotMeta,
) (*SnapshotMeta, error) {

	var created *SnapshotMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			created, err = s.createSnapshot(ctx, session, snapshot)
			return err
		},
	)
	return created, err
}

func (s *storageYDB) SnapshotCreated(
	ctx context.Context,
	snapshotID string,
	createdAt time.Time,
	snapshotSize uint64,
	snapshotStorageSize uint64,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.snapshotCreated(
				ctx,
				session,
				snapshotID,
				createdAt,
				snapshotSize,
				snapshotStorageSize,
			)
		},
	)
}

func (s *storageYDB) GetSnapshotMeta(
	ctx context.Context,
	snapshotID string,
) (*SnapshotMeta, error) {

	var snapshot *SnapshotMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			snapshot, err = s.getSnapshotMeta(ctx, session, snapshotID)
			return err
		},
	)
	return snapshot, err
}

func (s *storageYDB) DeleteSnapshot(
	ctx context.Context,
	snapshotID string,
	taskID string,
	deletingAt time.Time,
) (*SnapshotMeta, error) {

	var snapshot *SnapshotMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			snapshot, err = s.deleteSnapshot(ctx, session, snapshotID, taskID, deletingAt)
			return err
		},
	)
	return snapshot, err
}

func (s *storageYDB) SnapshotDeleted(
	ctx context.Context,
	snapshotID string,
	deletedAt time.Time,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.snapshotDeleted(ctx, session, snapshotID, deletedAt)
		},
	)
}

func (s *storageYDB) ClearDeletedSnapshots(
	ctx context.Context,
	deletedBefore time.Time,
	limit int,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.clearDeletedSnapshots(ctx, session, deletedBefore, limit)
		},
	)
}

func (s *storageYDB) LockSnapshot(
	ctx context.Context,
	snapshotID string,
	lockTaskID string,
) (locked bool, err error) {

	err = s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			locked, err = s.lockSnapshot(ctx, session, snapshotID, lockTaskID)
			return err
		},
	)
	return locked, err
}

func (s *storageYDB) UnlockSnapshot(
	ctx context.Context,
	snapshotID string,
	lockTaskID string,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.unlockSnapshot(ctx, session, snapshotID, lockTaskID)
		},
	)
}

func (s *storageYDB) ListSnapshots(
	ctx context.Context,
	folderID string,
	creatingBefore time.Time,
) ([]string, error) {

	var ids []string

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			ids, err = s.listSnapshots(ctx, session, folderID, creatingBefore)
			return err
		},
	)
	return ids, err
}

////////////////////////////////////////////////////////////////////////////////

func createSnapshotsYDBTables(
	ctx context.Context,
	folder string,
	db *persistence.YDBClient,
	dropUnusedColumns bool,
) error {

	logging.Info(ctx, "Creating tables for snapshots in %v", db.AbsolutePath(folder))

	err := db.CreateOrAlterTable(
		ctx,
		folder,
		"snapshots",
		snapshotStateTableDescription(),
		dropUnusedColumns,
	)
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created snapshots table")

	err = db.CreateOrAlterTable(
		ctx,
		folder,
		"incremental",
		persistence.NewCreateTableDescription(
			persistence.WithColumn("zone_id", persistence.Optional(persistence.TypeUTF8)),
			persistence.WithColumn("disk_id", persistence.Optional(persistence.TypeUTF8)),
			persistence.WithColumn("snapshot_id", persistence.Optional(persistence.TypeUTF8)),
			persistence.WithColumn("checkpoint_id", persistence.Optional(persistence.TypeUTF8)),
			persistence.WithPrimaryKeyColumn("zone_id", "disk_id"),
		),
		dropUnusedColumns,
	)
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created incremental table")

	err = db.CreateOrAlterTable(
		ctx,
		folder,
		"deleted",
		persistence.NewCreateTableDescription(
			persistence.WithColumn("deleted_at", persistence.Optional(persistence.TypeTimestamp)),
			persistence.WithColumn("snapshot_id", persistence.Optional(persistence.TypeUTF8)),
			persistence.WithPrimaryKeyColumn("deleted_at", "snapshot_id"),
		),
		dropUnusedColumns,
	)
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created deleted table")

	logging.Info(ctx, "Created tables for snapshots")

	return nil
}

func dropSnapshotsYDBTables(
	ctx context.Context,
	folder string,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Dropping tables for snapshots in %v", db.AbsolutePath(folder))

	err := db.DropTable(ctx, folder, "snapshots")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped snapshots table")

	err = db.DropTable(ctx, folder, "incremental")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped incremental table")

	err = db.DropTable(ctx, folder, "deleted")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped deleted table")

	logging.Info(ctx, "Dropped tables for snapshots")

	return nil
}
