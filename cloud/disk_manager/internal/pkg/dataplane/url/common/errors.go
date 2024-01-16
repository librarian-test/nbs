package common

import (
	"fmt"

	error_codes "github.com/ydb-platform/nbs/cloud/disk_manager/pkg/client/codes"
	"github.com/ydb-platform/nbs/cloud/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func NewSourceNotFoundError(format string, args ...interface{}) error {
	return errors.NewDetailedError(
		fmt.Errorf(format, args...),
		&errors.ErrorDetails{
			Code:     error_codes.BadSource,
			Message:  "url source not found",
			Internal: false,
		},
	)
}

func NewSourceInvalidError(format string, args ...interface{}) error {
	return errors.NewDetailedError(
		fmt.Errorf(format, args...),
		&errors.ErrorDetails{
			Code:     error_codes.BadSource,
			Message:  "url source invalid",
			Internal: false,
		},
	)
}
