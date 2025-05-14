import pytest
from .nebius-populate-vms import decide_scaling

@pytest.mark.parametrize("matched, idle, busy, to_remove, max_create, max_total, extra, expected_create", [
    ([], [], [], [], 1, 5, 1, 1),  # No VMs at all, should create one
    (["vm1"], ["vm1"], [], [], 1, 5, 1, 0),  # 1 idle already present
    (["vm1"], [], ["vm1"], [], 1, 5, 1, 0),  # 1 busy VM, already fine
    (["vm1"], ["vm1"], [], ["vm1"], 1, 5, 1, 1),  # 1 idle VM removed by TTL, should be replaced
    (["vm1", "vm2"], [], ["vm1", "vm2"], [], 1, 5, 1, 1),  # 2 busy, can add 1 extra
    (["vm1", "vm2", "vm3"], ["vm1", "vm2"], ["vm3"], [], 1, 5, 1, 0),  # Too many idle, should downscale only
])
def test_decide_scaling(matched, idle, busy, to_remove, max_create, max_total, extra, expected_create):
    remove_copy = list(to_remove)  # simulate pass-by-reference
    to_create, projected_vm_count, excess_idle = decide_scaling(
        matched, idle, busy, remove_copy,
        max_create, max_total, extra
    )
    assert to_create == expected_create
