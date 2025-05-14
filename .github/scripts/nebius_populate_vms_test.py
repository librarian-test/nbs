import pytest
from .nebius_populate_vms import decide_scaling

from dataclasses import dataclass


@dataclass
class ScalingTestCase:
    id: str
    matched: list
    idle: list
    busy: list
    to_remove: list
    max_create: int
    max_total: int
    extra: int
    expected_create: int


@pytest.mark.parametrize(
    "case",
    [
        pytest.param(
            ScalingTestCase(
                id="no-vms-create-one",
                matched=[],
                idle=[],
                busy=[],
                to_remove=[],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=1,
            ),
        ),
        pytest.param(
            ScalingTestCase(
                id="idle-vm-sufficient",
                matched=["vm1"],
                idle=["vm1"],
                busy=[],
                to_remove=[],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=0,
            ),
        ),
        pytest.param(
            ScalingTestCase(
                id="one-busy-vm-no-create",
                matched=["vm1"],
                idle=[],
                busy=["vm1"],
                to_remove=[],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=0,
            ),
        ),
        pytest.param(
            ScalingTestCase(
                id="idle-vm-removed-by-ttl",
                matched=["vm1"],
                idle=["vm1"],
                busy=[],
                to_remove=["vm1"],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=1,
            ),
        ),
        pytest.param(
            ScalingTestCase(
                id="busy-vms-allow-extra",
                matched=["vm1", "vm2"],
                idle=[],
                busy=["vm1", "vm2"],
                to_remove=[],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=1,
            ),
        ),
        pytest.param(
            ScalingTestCase(
                id="too-many-idle-no-create",
                matched=["vm1", "vm2", "vm3"],
                idle=["vm1", "vm2"],
                busy=["vm3"],
                to_remove=[],
                max_create=1,
                max_total=5,
                extra=1,
                expected_create=0,
            ),
        ),
    ],
)
def test_decide_scaling(case):
    remove_copy = list(case.to_remove)  # simulate pass-by-reference
    to_create, projected_vm_count, excess_idle = decide_scaling(
        case.matched,
        case.idle,
        case.busy,
        remove_copy,
        case.max_create,
        case.max_total,
        case.extra,
    )
    assert to_create == case.expected_create
