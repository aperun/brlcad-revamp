# BetterCAD

  Modular CAD System that should be compatible with STEP standard.

## DOCUMENTATION<a name="docs"></a>

  For specific documentation like Compiling, GUI translation, old changelogs see
   the Documentation subfolder.

## META-FILES<a name="meta"></a>

  AUTHORS       - The authors, contributors, document writers and translators list
  BUILD         - The instructions to build releases (binaries)
  INSTALL       - The instructions to install releases (binaries)
  LICENSE       - A modified copy of the BSD 4-Clause License to be linked to in source comments
  TODO          - Project-wide Todo list

## Submodules
  app/*         - 
  common        - Common library
  ext           - Extension library
  help          - Help Tool
  icon          - Application icon resources (PNG, SVG, etc)
  renderer      - Visualizer for both 3D & 2D imagery
bitmap2component  - Sourcecode of the bitmap to pcb artwork converter
bitmaps_png       - Menu and program icons
cvpcb             - Sourcecode of the CvPCB tool
eeschema          - Sourcecode of the schematic editor
gerbview          - Sourcecode of the gerber viewer
helpers           - Helper tools and utilities for development
kicad             - Sourcecode of the project manager
lib_dxf           - Sourcecode of the dxf reader/writer library
new               - Staging area for the new schematic library format
pagelayout_editor - Sourcecode of the pagelayout editor
patches           - Collection of patches for external dependencies
pcbnew            - Sourcecode of the printed circuit board editor
plugins           - Sourcecode of the new plugin concept
polygon           - Sourcecode of the polygon library
potrace           - Sourcecode of the potrace library, used in bitmap2component
qa                - Testcases using the python interface
resources         - Resources for freedesktop mime-types for linux
scripting         - SWIG Python scripting definitions and build scripts
scripts           - Example scripts for distribution with KiCad
template          - Project and pagelayout templates
tools             - Other miscellaneous helpers for testing
utils             - Small utils for kicad, e.g. IDF tools

## OTHER DIRECTORIES
|||
----------|-----------------
  demo    |  Demo examples
  doc     |  Documentation for end-users, developers, and system admins
  test    |  Tests for release builds