# Fork Merge Log

Running record of merges from upstream `meshcore-dev/MeshCore` into this fork
(`nrf-txt-fw`). The point is to remember *why* a conflict was resolved the way it
was, so the next merge doesn't silently undo a deliberate decision.

**Read the "Standing divergences" section before starting a merge.** Those are the
places that conflict every time.

## How to merge

```sh
git fetch upstream --tags
git merge-tree --write-tree --name-only HEAD <upstream-tag>   # dry run: lists conflicts
git merge <upstream-tag>
```

After resolving, build the fork target before trusting the result — several
breakages don't show up as conflicts, because they are clean upstream edits to
APIs the fork's own code depends on:

```sh
pio run -e Heltec_t114_nrf_txt
```

## Standing divergences

Fork-specific code that upstream doesn't have. Expect these to conflict.

### `hasSeenPacket()` wrapper + `out_hash` plumbing
- **Files:** `src/Mesh.h`, `src/Mesh.cpp`, `src/helpers/SimpleMeshTables.h`
- **What:** fork keeps `Mesh::hasSeenPacket(pkt, out_hash)` and the
  `onOwnPacketTracked()` / `onSeenDuplicatePacket()` virtual hooks. Upstream has no
  equivalent.
- **Why:** these drive the nrf-txt "heard repeat" 3-dot indicator on outgoing
  messages. `out_hash` hands the packet hash to the UI so it can match an echo back
  to a specific message in the list.
- **Care:** upstream split `MeshTables::hasSeen()` into pure `wasSeen()` +
  explicit `markSeen()`. The fork keeps the combined wrapper on top of that split —
  `wasSeen()` gained the optional `out_hash` param, and the wrapper calls
  `markSeen()` itself on a miss. Keep both sides in sync or dedup silently breaks.

### `NodePrefs.tz_offset`
- **Files:** `examples/companion_radio/NodePrefs.h`, `examples/companion_radio/DataStore.cpp`
- **What:** fork-only timezone offset, serialized as `def("tz", tz_offset)`.
- **Care:** as of v1.17.1 prefs are named key/value JSON (`ConfigSerializer`), so new
  upstream fields no longer disturb fork fields. The old fixed-offset
  `_reserved_upstream[128]` padding hack is gone and should not come back.

### `DisplayDriver::Color` enum
- **File:** `src/helpers/ui/DisplayDriver.h`
- **What:** fork keeps the `Color` enum (and adds `INVERSE`); upstream commented it
  out in favour of `ColorVal` (`uint16_t`) + the `UIColor` palette class.
- **Why:** the nrf-txt UI uses `DisplayDriver::LIGHT` / `DARK` / `INVERSE`
  throughout. The enum values implicitly convert to `ColorVal`, so both coexist.

### The entire nrf-txt UI
- **Dir:** `examples/companion_radio/nrf-txt/`
- **What:** custom keyboard/screen UI. Upstream never touches these files, so they
  never conflict — but they consume upstream APIs (`AbstractUITask`,
  `DisplayDriver`, `BaseChatMesh`) and **break silently when those change.**
  This is the main source of post-merge build failures.

### `platformio.ini` / variant pins
- Fork pins some platform/library versions and adds `inject_build_time.py`.
  Re-check against upstream's values each merge.

### Per-channel send scope (known gap, not a bug to fix on merge)
- `MyMesh::sendFloodScoped(GroupChannel&, ...)` carries a
  `TODO: have per-channel send_scope`. The radio's `ChannelDetails` has nowhere to
  store a per-channel scope, so the device UI falls back to
  `_prefs.default_scope_key` for every channel. The phone app instead pushes scope
  per-send via `CMD_SET_FLOOD_SCOPE_KEY`, so app and device-UI sends can differ.
- **Consequence:** if `default_scope_key` holds a key no repeater matches, every
  device-UI channel message is dropped by repeaters — silently, 100% of the time,
  while app-sent messages work. Relying on the default scope is the accepted
  design; just make sure it is set correctly (app settings → default scope).

---

## Merges

### v1.17.1 — upstream `d9296435` (2026-08-14), merged 2026-09-18

Merge base `07a3ca9e` (v1.16.0). 367 upstream commits, 10 conflicting files.

**Conflict resolutions**

- `src/Mesh.h` / `src/Mesh.cpp` / `src/helpers/SimpleMeshTables.h` — adopted
  upstream's `wasSeen()` / `markSeen()` split, but kept the fork's `hasSeenPacket()`
  wrapper and `out_hash` plumbing on top (see Standing divergences).
- `src/helpers/SimpleMeshTables.h` — **dropped the fork's separate 64-entry ACK
  table** and adopted upstream's single unified hash table (`MAX_PACKET_HASHES`
  128 → 160). Reverses the v1.16 decision, deliberately: the fork's ACK table keyed
  on a 4-byte ACK CRC, which predates upstream's 5-byte ACK support and silently
  truncated those. The unified table is length-agnostic. Net dedup capacity
  192 → 160 entries; judged acceptable.
- `examples/companion_radio/NodePrefs.h` + `DataStore.cpp` — adopted upstream's
  `ConfigSerializer` named key/value prefs, replacing the fork's raw byte-offset
  struct. `tz_offset` re-added as `def("tz", ...)`. Legacy `/new_prefs` binary
  reader kept for one-time migration; the old `_reserved_upstream[128]` padding is
  read into a throwaway buffer and discarded.
- `src/helpers/ui/DisplayDriver.h` — kept the fork's `Color` enum alongside
  upstream's new `ColorVal` / `UIColor`.
- `examples/companion_radio/AbstractUITask.h` — took upstream's
  `MultiSerialInterface`, kept the fork's `ContactInfo.h` include.
- `examples/companion_radio/MyMesh.cpp` — additive; kept fork's `tz_offset` default
  alongside upstream's new `radio_fem_rxgain` / `radio_fem_txgain` defaults.
- `src/helpers/sensors/MicroNMEALocationProvider.h` — took upstream's member
  init order (silences a `-Wreorder` warning), kept fork's formatting.
- `platformio.ini` — took upstream's `CustomLFS` 0.2.3 and the new `[env:native]` /
  `[env:native_kiss_modem]` test envs.

**Silent breakages — clean upstream edits that broke fork code without conflicting**

These cost the most time. All were build or runtime failures with no merge conflict:

1. **Contact list truncation (runtime, no build error).** Upstream now reserves the
   first `MAX_ANON_CONTACTS` (8) array slots for anon contacts, changed
   `getNumContacts()` to exclude them, and added `getTotalContactSlots()` for the raw
   bound. `getContactByIdx()` still takes a raw index. The fork's
   `ui_task.cpp` looped `0..getNumContacts()` over raw indices, so it wasted 8
   iterations on reserved slots and **dropped the last 8 contacts**. Fixed in
   `findContactIndexByPrefix()` and `getContactIndexes()`.
2. **`Color` → `ColorVal`.** `nrf_hardware.h/.cpp` overrides still had `Color`
   parameters, so they no longer overrode the (now abstract) base methods.
3. **`UIColor` statics undefined at link time.** Every display driver defines its
   own palette; the fork's custom driver had none. Added to `nrf_hardware.cpp`.
4. **`BaseSerialInterface` → `MultiSerialInterface`.** `ui_task.h` ctor param, and
   `isSerialEnabled()` / `enableSerial()` / `disableSerial()` →
   `isBluetoothEnabled()` / `enableBluetooth()` / `disableBluetooth()`.
5. **`NodePrefs.client_repeat` → `isRepeatEn()` / `setRepeatEn()`.** Fork's camp-mode
   toggle in `ui_task.cpp` referenced the removed field.

**Verified not broken** (investigated at length chasing a reported "3-dot indicator
stopped working", which turned out to be a bad stored `default_scope_key`, not a
merge regression):

- `ConfigSerializer` prefs round-trip is byte-exact, including `defs_key` and the
  fork's `tz` field appended after the nested sub-objects. Checked with a native
  harness against `test/mocks`.
- Legacy `/new_prefs` binary read sequence is byte-identical pre- and post-merge.
- Send path is unchanged: scope selection, packet header/length, and
  `startSendRaw()` all behave as before.

**Downgrade hazard:** flashing pre-merge firmware onto a device that has run
post-merge firmware can crash on boot — `CustomLFS` went 0.2.1 → 0.2.3. Don't A/B
test across that boundary on a device with a populated filesystem.

### v1.16.0 — upstream `07a3ca9e`, merged as `775256cb`

Recorded from the merge commit message:

- `NodePrefs` / `DataStore` — kept both fork `tz_offset` and upstream
  `default_scope_name` / `default_scope_key`. Added a 128-byte `_reserved_upstream`
  buffer between upstream and fork prefs so future upstream additions wouldn't shift
  `tz_offset`'s file offset. *(Superseded in v1.17.1 by named key/value prefs.)*
- `SimpleMeshTables.h` — kept the fork's separate ACK tracking and `hasSeen`
  `out_hash` plumbing (needed by the outgoing-heard-icon feature).
  *(ACK table later dropped in v1.17.1 — see above.)*
- `buzzer.cpp` / `buzzer.h` — kept fork's `begin(play_startup)` toggle.
- `platformio.ini` — kept fork's nrf52/stm32 pinned versions and
  `inject_build_time.py` extra script.
