# Copyright (c) 2020-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This cmake-format configuration file is a suggested configuration file for
# formatting CMake files for the HPX project.

# PLEASE NOTE: This file has been created and tested with cmake-format V0.6.10

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # If true, separate function names from parentheses with a space
  separate_fn_name_with_space = False

  # Format command names consistently as 'lower' or 'upper' case
  command_case = u'lower'

  # If the statement spelling length (including space and parenthesis) is
  # larger
  # than the tab width by more than this amount, then force reject un-nested
  # layouts.
  max_prefix_chars = 10

  # If the trailing parenthesis must be 'dangled' on its own line, then align
  # it
  # to this reference: `prefix`: the start of the statement, `prefix-indent`:
  # the start of the statement, plus one indentation level, `child`: align to
  # the column of the arguments
  dangle_align = u'prefix'

  # If an argument group contains more than this many sub-groups (parg or kwarg
  # groups) then force it to a vertical layout.
  max_subgroups_hwrap = 2

  # If the statement spelling length (including space and parenthesis) is
  # smaller than this amount, then force reject nested layouts.
  min_prefix_chars = 4

  # If a positional argument group contains more than this many arguments, then
  # force it to a vertical layout.
  max_pargs_hwrap = 6

  # If a candidate layout is wrapped horizontally but it exceeds this many
  # lines, then reject the layout.
  max_lines_hwrap = 2

  # If true, the parsers may infer whether or not an argument list is sortable
  # (without annotation).
  autosort = False

  # What style line endings to use in the output.
  line_ending = u'auto'

  # How wide to allow formatted cmake files
  line_width = 80

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = True

  # How many spaces to tab for indent
  tab_size = 2

  # A list of command names which should always be wrapped
  always_wrap = []

  # If true, separate flow control names from their parentheses with a space
  separate_ctrl_name_with_space = False

  # If a cmdline positional group consumes more than this many lines without
  # nesting, then invalidate the layout (and nest)
  max_rows_cmdline = 2

  # By default, if cmake-format cannot successfully fit everything into the
  # desired linewidth it will apply the last, most aggressive attempt that it
  # made.  If this flag is True, however, cmake-format will print error, exit
  # with non-zero status code, and write-out nothing
  require_valid_layout = False

  # Format keywords consistently as 'lower' or 'upper' case
  keyword_case = u'unchanged'

  # If true, the argument lists which are known to be sortable will be sorted
  # lexicographicall
  enable_sort = True

  # A dictionary mapping layout nodes to a list of wrap decisions.  See the
  # documentation for more information.
  layout_passes = {}

# ------------------------------------------------
# Options affecting comment reflow and formatting.
# ------------------------------------------------
with section("markup"):

  # If comment markup is enabled, don't reflow any comment block which matches
  # this (regex) pattern.  Default is `None` (disabled).
  literal_comment_pattern = None

  # If a comment line starts with at least this many consecutive hash
  # characters, then don't lstrip() them off.  This allows for lazy hash rulers
  # where the first hash char is not separated by space
  hashruler_min_length = 10

  # Regular expression to match preformat fences in comments default=
  # ``r'^\s*([`~]{3}[`~]*)(.*)$'``
  fence_pattern = u'^\\s*([`~]{3}[`~]*)(.*)$'

  # If true, then insert a space between the first hash char and remaining hash
  # chars in a hash ruler, and normalize its length to fill the column
  canonicalize_hashrulers = True

  # If a comment line matches starts with this pattern then it is explicitly a
  # trailing comment for the preceding argument.  Default is '#<'
  explicit_trailing_pattern = u'#<'

  # If comment markup is enabled, don't reflow the first comment block in each
  # listfile.  Use this to preserve formatting of your copyright/license
  # statements.
  first_comment_is_literal = True

  # enable comment markup parsing and reflow
  enable_markup = True

  # Regular expression to match rulers in comments default=
  # ``r'^\s*[^\w\s]{3}.*[^\w\s]{3}$'``
  ruler_pattern = u'^\\s*[^\\w\\s]{3}.*[^\\w\\s]{3}$'

  # What character to use as punctuation after numerals in an enumerated list
  enum_char = u'.'

  # What character to use for bulleted lists
  bullet_char = u'*'

# ----------------------------
# Options affecting the linter
# ----------------------------
with section("lint"):

  # regular expression pattern describing valid function names
  function_pattern = u'[0-9a-z_]+'

  # regular expression pattern describing valid names for function/macro
  # arguments and loop variables.
  argument_var_pattern = u'[a-z][a-z0-9_]+'

  # a list of lint codes to disable
  disabled_codes = []

  # Require at least this many newlines between statements
  min_statement_spacing = 1

  # regular expression pattern describing valid macro names
  macro_pattern = u'[0-9A-Z_]+'

  # regular expression pattern describing valid names for public directory
  # variables
  public_var_pattern = u'[A-Z][0-9A-Z_]+'
  max_statements = 50

  # In the heuristic for C0201, how many conditionals to match within a loop in
  # before considering the loop a parser.
  max_conditionals_custom_parser = 2

  # regular expression pattern describing valid names for variables with global
  # (cache) scope
  global_var_pattern = u'[A-Z][0-9A-Z_]+'

  # regular expression pattern describing valid names for keywords used in
  # functions or macros
  keyword_pattern = u'[A-Z][0-9A-Z_]+'
  max_arguments = 5

  # regular expression pattern describing valid names for privatedirectory
  # variables
  private_var_pattern = u'_[0-9a-z_]+'
  max_localvars = 15
  max_branches = 12

  # regular expression pattern describing valid names for variables with local
  # scope
  local_var_pattern = u'[a-z][a-z0-9_]+'

  # Require no more than this many newlines between statements
  max_statement_spacing = 2

  # regular expression pattern describing valid names for variables with global
  # scope (but internal semantic)
  internal_var_pattern = u'_[A-Z][0-9A-Z_]+'
  max_returns = 6

# -------------------------------------
# Miscellaneous configurations options.
# -------------------------------------
with section("misc"):

  # A dictionary containing any per-command configuration overrides.  Currently
  # only `command_case` is supported.
  per_command = { }

# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section("parse"):

  # Specify structure for custom cmake functions
  # (the body of this structure was generated using
  #     'cmake-genparsers -f python cmake/*.cmake'
  #
  additional_commands = { 'add_chplx_pseudo_dependencies': {'pargs': {'nargs': 0}},
  'add_chplx_pseudo_dependencies_no_shortening': {'pargs': {'nargs': 0}},
  'add_chplx_pseudo_target': {'pargs': {'nargs': 0}},
  'add_chplx_pseudo_target_folder': { 'kwargs': { 'FOLDER': 1,
                                                  'TARGET': 1 },
                                      'pargs': {'nargs': 0}},
  'add_chplx_test': { 'kwargs': { 'ARGS': '+',
                                  'EXECUTABLE': 1,
                                  'LOCALITIES': 1,
                                  'PARCELPORTS': '+',
                                  'RUNWRAPPER': 1,
                                  'THREADS_PER_LOCALITY': 1,
                                  'TIMEOUT': 1},
                      'pargs': { 'flags': [ 'FAILURE_EXPECTED',
                                            'RUN_SERIAL',
                                            'NO_PARCELPORT_TCP',
                                            'NO_PARCELPORT_MPI',
                                            'NO_PARCELPORT_LCI'],
                                 'nargs': '2+'}},
  'chplx_compare_result': { 'kwargs': { 'GOOD': 1,
                                        'TEST_NAME': 1,
                                        'DEPENDENCIES': '+',
                                        'SOURCES': '+'},
                            'pargs': {'nargs': '2+'}},
  'chplx_compile_project': { 'kwargs': { 'TARGET': 1,
                                         'TEST_NAME': 1,
                                         'DEPENDENCIES': '+'},
                            'pargs': {'nargs': '2+'}},
  'chplx_format_file': { 'kwargs': { 'SOURCES': '+',
                                     'DEPENDENCIES': '+'},
                         'pargs': {'nargs': '2+'}},
  'chplx_config_loglevel': {'pargs': {'nargs': 2}},
  'chplx_debug': {'pargs': {'nargs': 0}},
  'chplx_error': {'pargs': {'nargs': 0}},
  'chplx_include': {'pargs': {'nargs': 0}},
  'chplx_info': {'pargs': {'nargs': 0}},
  'chplx_message': {'pargs': {'nargs': 1}},
  'chplx_option': { 'kwargs': {'CATEGORY': 1, 'MODULE': 1, 'STRINGS': '+'},
                    'pargs': {'flags': ['ADVANCED'], 'nargs': '4+'}},
  'chplx_print_list': {'pargs': {'nargs': 3}},
  'chplx_set_option': { 'kwargs': {'HELPSTRING': 1, 'TYPE': 1, 'VALUE': 1},
                        'pargs': {'flags': ['FORCE'], 'nargs': '1+'}},
  'chplx_warn': {'pargs': {'nargs': 0}},
  'shorten_chplx_pseudo_target': {'pargs': {'nargs': 2}}}

  # Specify property tags.
  proptags = []

  # Specify variable tags.
  vartags = []

# -------------------------------
# Options affecting file encoding
# -------------------------------
with section("encode"):

  # If true, emit the unicode byte-order mark (BOM) at the start of the file
  emit_byteorder_mark = False

  # Specify the encoding of the input file.  Defaults to utf-8
  input_encoding = u'utf-8'

  # Specify the encoding of the output file.  Defaults to utf-8.  Note that
  # cmake
  # only claims to support utf-8 so be careful when using anything else
  output_encoding = u'utf-8'

