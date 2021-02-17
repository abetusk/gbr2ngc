Develoepr Documentation
===

This is a short guide to the code, how it's organized, motivation and how to extend and contribute.

Still a work in progress.

Improvements welcome!

Overview
---

As a reminder, the goal is to parse a Gerber file and output GCode.

* `gbr2ngc.cpp` - main program file
* `gerber_interpretr.c` - parse Gerber file
* `gbr2ngc_aperture.cpp` - realize aperture geometry
* `gbr2ngc_construct.cpp` - realize end geometry
* `gbr2ngc_export.cpp` - export to GCode fuctions
* `gerber_interpretr_aperture_macro.c` - helper functions for Gerber aperture macro parsing
* `clipper.cpp` - ClipperLib (3rd party library for boolean geometry operations)
* `tesexpr.c` - tiny expression (3rd party library for expression handling for Gerber processing)

`gbr2ngc.cpp` has `main` and does the following high level steps:

* Process command lines (`process_command_line_options`)
* Parse/tokenize Gerber file (`gerber_state_load_file`)
* Realize apertures (`realize_apertures`)
* Realize geometry (`join_polygon_set`)
* Apply polygon offsetting if necessary (`construct_polygon_offset`)
* Export to GCode (`export_paths_to_gcode_unit`)


Gerber Processing
---

As an attempt at creating a stand-alone straight C implementation of a Gerber interpreter,
the Gerber parsing has been limited to the following C files:

* `gerber_interpreter.h`
* `gerber_interpreter.c`
* `gerber_interpreter_aperture_macro.c`

`gerber_state_load_file` is the main function to do the processing.
As a brief overview:

* A line buffer is allocated (`linebuf`) with size `GERBER_STATE_LINEBUF`
* Each line is read and processed with `munch_line`
* A successful return of `munch_line` means there is a line to be interpreted
* Each collected line is processed with `gerber_stae_interpret_line`
* `gerber_state_post_process` is done for any post-processing

`munch_line` strips whitespace for ease of processing later.

`gerber_state_interpret_line` processed the lines and does the dispatch to the
other paring functions (perhaps later used as callbacks?).

For multi-line commands, the comamnds should hold state about where they are
to communicate state in the `gerber_read_state` struct variable.
Because the added Gerber commands essentially add 'functions' to the Gerber
spec, we need to have a recursive structure to hold the state of the Gerber parsing.

In `gerber_state_interpret_line`, the 'active' gerber state (`root_gs->absr_lib_active_gs`)
is used as the current gerber state (the local `gs` variable).
Functions that are finished with the context have the job of updating the `absr_lib_active_gs`.



Gerber process todo
---

* ACTIVE: I think the best way forward is to replace the `contour_ll_t` structure with `gerber_item_ll_t` in all
  places it appears.

* add callbacks to the structure for overloading
* add callback for error

