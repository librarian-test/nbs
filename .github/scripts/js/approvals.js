const org            = 'ydb-platform';
const nebiusTeamSlug = 'nbs_nebius';
const yandexTeamSlug = 'nbs_yandex';

const exemptLeftFolders  = [
    /* paths that remove the *left-team* requirement */
];

const exemptRightFolders = [
    /* paths that remove the *right-team* requirement */
    'cloud/blockstore/tools/csi_driver/',
    'cloud/blockstore/tests/csi_driver/',
    '.github/',
];

const leftUsersData  = await github.paginate(
    github.rest.teams.listMembersInOrg, { org, team_slug: nebiusTeamSlug }
);
const rightUsersData = await github.paginate(
    github.rest.teams.listMembersInOrg, { org, team_slug: yandexTeamSlug }
);

const leftUsers  = leftUsersData .map(u => u.login);
const rightUsers = rightUsersData.map(u => u.login);

const superUsers = ['SvartMetal', 'EvgeniyKozev'];

const { owner, repo } = context.repo;
const prNumber        = context.issue.number;

const changedFiles = await github.paginate(
    github.rest.pulls.listFiles,
    { owner, repo, pull_number: prNumber, per_page: 100 },
    res => res.data
);

const needsLeftApproval = changedFiles.some(f =>
    !exemptLeftFolders.some(folder => f.filename.startsWith(folder))
);

const needsRightApproval = changedFiles.some(f =>
    !exemptRightFolders.some(folder => f.filename.startsWith(folder))
);

console.log(
    `PR #${prNumber} touches:` +
    `\n  • Any left-exempt paths?  ${needsLeftApproval  ? 'NO' : 'YES'} ` +
    `\n  • Any right-exempt paths? ${needsRightApproval ? 'NO' : 'YES'} ` +
    `\n→ Left approval ${needsLeftApproval  ? 'REQUIRED' : 'NOT required'}` +
    `\n→ Right approval ${needsRightApproval ? 'REQUIRED' : 'NOT required'}`
);

const isInList = (user, list) => list.includes(user);

if (
    context.eventName === 'issue_comment' &&
    isInList(context.actor, superUsers) &&
    context.payload.comment.body.trim() === '/approve'
) {
    console.log('Approval bypassed by super user.');
    return;
}

if (context.eventName !== 'pull_request_review') {
    core.setFailed('Not enough approvals');
    return;
}

const reviews = await github.paginate(
    github.rest.pulls.listReviews,
    { owner, repo, pull_number: prNumber },
    r => r.data
);

const leftApprovals  = new Set();
const rightApprovals = new Set();

for (const r of reviews) {
    if (r.state === 'APPROVED') {
        if (isInList(r.user.login, leftUsers))  leftApprovals.add(r.user.login);
        if (isInList(r.user.login, rightUsers)) rightApprovals.add(r.user.login);
    }
    if (r.state === 'DISMISSED') {
        leftApprovals.delete(r.user.login);
        rightApprovals.delete(r.user.login);
    }
}

const hasLeft  = leftApprovals.size  >= 1 || !needsLeftApproval;
const hasRight = rightApprovals.size >= 1 || !needsRightApproval;

if (hasLeft && hasRight) {
    console.log('✅ PR meets approval requirements.');
} else {
    console.log('❌ PR does not meet approval requirements.');
    console.log(`Left approvals : ${[...leftApprovals ].join(', ') || 'none'}`);
    console.log(`Right approvals: ${[...rightApprovals].join(', ') || 'none'}`);
    core.setFailed('PR does not meet approval requirements.');
}
