## ADDED Requirements

### Requirement: Draft configuration editing
The system SHALL allow users to create and edit configurations in draft state without affecting live (published) configurations. Draft changes MUST be isolated per namespace.

#### Scenario: Edit config in draft
- **WHEN** a user edits config key `timeout` in namespace `order-service/DEV/default/application` while in draft mode
- **THEN** the change is saved as a draft and does NOT affect clients reading the published config

#### Scenario: Multiple draft edits
- **WHEN** a user makes three edits to different keys in the same namespace while in draft mode
- **THEN** all three changes are stored as draft and can be published together as a single release

### Requirement: Publish release
The system SHALL support publishing a draft into a release. Publishing MUST move all draft changes to the live configuration and trigger notifications to subscribed clients.

#### Scenario: Publish a draft release
- **WHEN** a user publishes a draft containing changes to keys `timeout` and `retries`
- **THEN** both keys are updated to their new values in the live config, a release record is created with a release note, and subscribed clients receive change notifications

### Requirement: Release notes
The system SHALL allow the publisher to attach a release note (plain text, max 4096 characters) describing the changes in the release.

#### Scenario: Publish with release note
- **WHEN** a user publishes with release note "Increased timeout for payment gateway"
- **THEN** the release record includes the note and it is visible in the release history

### Requirement: Approval workflow (optional)
The system SHALL support an optional approval workflow. When enabled for a namespace, a draft MUST be approved by a designated approver before it can be published. When disabled, any authorized user can publish directly.

#### Scenario: Publish with approval required
- **WHEN** approval is enabled for a namespace and a user submits a draft for approval
- **THEN** the draft enters "pending approval" state, and only an approver can approve and publish it

#### Scenario: Publish without approval
- **WHEN** approval is disabled for a namespace
- **THEN** any authorized user can publish a draft directly without approval

### Requirement: Grayscale (canary) release
The system SHALL support grayscale releases where a new configuration version is delivered to a subset of client instances before full rollout. Rules SHALL support percentage-based rollout and explicit IP/instance-id whitelist.

#### Scenario: Percentage-based grayscale
- **WHEN** a user publishes a config with grayscale rule "30% of instances"
- **THEN** approximately 30% of subscribing clients receive the new config version, and 70% continue receiving the previous version

#### Scenario: Whitelist-based grayscale
- **WHEN** a user publishes a config with grayscale rule targeting instances `["192.168.1.10", "instance-abc"]`
- **THEN** only those two instances receive the new config version; all others receive the previous version

#### Scenario: Full rollout after grayscale
- **WHEN** a user completes a full rollout after a successful grayscale release
- **THEN** all instances receive the new config version, and the old version is retired

### Requirement: Release history
The system SHALL maintain a release history for each namespace, showing all published releases with their version, timestamp, operator, release note, and whether it was a full or grayscale release.

#### Scenario: View release history
- **WHEN** a user requests the release history for namespace `order-service/DEV/default/application`
- **THEN** the system returns all past releases in reverse chronological order
