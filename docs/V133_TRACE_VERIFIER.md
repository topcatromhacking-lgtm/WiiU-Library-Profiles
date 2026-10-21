# WULP v1.3.3 active-use trace gate (PC-only, synthetic reference)

The v1.3.2 report proved the SD Barista candidate file was readable and intact. It did **not** show the Wii U Menu reading through the active redirect. v1.3.3 provides the fail-closed *offline verification algorithm* for future instrumented traces.

## Inputs

- Console-generated, separately reviewed, authentic, one-boot JSONL file containing ordered `SAVE_INIT`, `LAYER_REGISTER`, `MENU_OPEN`, `MENU_READ`, `MENU_CLOSE`, `LAYER_REMOVE`, `SESSION_END` events.
- One independently backed-up 130,116-byte Barista file held locally on the owner's PC.
- Private Wii U user account ID (do not publish it).

The program expects exact event keys. The correct answer is **unproven** if there are no observed reads through the real ContentRedirectionModule, a short or mismatched read, an early close/remove, or an invalid account session.

Example only after trusted instrumentation exists:

```sh
python src/v133_active_use_trace.py --trace /private/actual-menu-read-events.jsonl --barista-backup /private/Barista-copy1.dat --account 8XXXXXXX
```

Successful reference output is `ACTIVE_USE_VERIFIED_BY_TRACE` when all chunks collectively cover the original backed-up file, their SHA-256 digests match the corresponding sections, and the trace is internally consistent. **This is a claimed result of the *trace evidence*, not an external proof that the trace was gathered from a Wii U.** A synthetic test deliberately produces the same label for functional coverage. Only a trusted, versioned Wii U instrumented emitter can validate the claim that the events correspond to actual hardware I/O.

Test without a Wii U (uses generated synthetic image in a temporary directory):

```sh
python -m unittest discover -s tests -v
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_v133_trace_capture.c -o /tmp/wulp-v133-capture
/tmp/wulp-v133-capture
```

The portable header does not depend on Wii U libraries and is **not** a console installer. Do not place the test binary in the Aroma plugin folder. No Barista or WLPA files are modified by this tool.

See [integration planning](ACTIVE_MENU_REDIRECTION_GATE.md) for the exact upstream API points and safety preconditions.
