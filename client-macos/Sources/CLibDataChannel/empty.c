// Swift Package Manager requires a C target to have at least one source
// file to compile. This target's only real job is exposing shim.h (and
// through it, libdatachannel's rtc.h) to Swift — there's no actual C code
// of ours to write, so this file is intentionally empty.
