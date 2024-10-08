// Copyright The OpenTelemetry Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package x

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestExemplars(t *testing.T) {
	const key = "OTEL_GO_X_EXEMPLAR"
	require.Equal(t, key, Exemplars.Key())

	t.Run("true", run(setenv(key, "true"), assertEnabled(Exemplars, "true")))
	t.Run("True", run(setenv(key, "True"), assertEnabled(Exemplars, "True")))
	t.Run("TRUE", run(setenv(key, "TRUE"), assertEnabled(Exemplars, "TRUE")))
	t.Run("false", run(setenv(key, "false"), assertDisabled(Exemplars)))
	t.Run("1", run(setenv(key, "1"), assertDisabled(Exemplars)))
	t.Run("empty", run(assertDisabled(Exemplars)))
}

func TestCardinalityLimit(t *testing.T) {
	const key = "OTEL_GO_X_CARDINALITY_LIMIT"
	require.Equal(t, key, CardinalityLimit.Key())

	t.Run("100", run(setenv(key, "100"), assertEnabled(CardinalityLimit, 100)))
	t.Run("-1", run(setenv(key, "-1"), assertEnabled(CardinalityLimit, -1)))
	t.Run("false", run(setenv(key, "false"), assertDisabled(CardinalityLimit)))
	t.Run("empty", run(assertDisabled(CardinalityLimit)))
}

func run(steps ...func(*testing.T)) func(*testing.T) {
	return func(t *testing.T) {
		t.Helper()
		for _, step := range steps {
			step(t)
		}
	}
}

func setenv(k, v string) func(t *testing.T) {
	return func(t *testing.T) { t.Setenv(k, v) }
}

func assertEnabled[T any](f Feature[T], want T) func(*testing.T) {
	return func(t *testing.T) {
		t.Helper()
		assert.True(t, f.Enabled(), "not enabled")

		v, ok := f.Lookup()
		assert.True(t, ok, "Lookup state")
		assert.Equal(t, want, v, "Lookup value")
	}
}

func assertDisabled[T any](f Feature[T]) func(*testing.T) {
	var zero T
	return func(t *testing.T) {
		t.Helper()

		assert.False(t, f.Enabled(), "enabled")

		v, ok := f.Lookup()
		assert.False(t, ok, "Lookup state")
		assert.Equal(t, zero, v, "Lookup value")
	}
}
