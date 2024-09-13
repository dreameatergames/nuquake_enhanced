project('GLdc', 'c',
  version : '0.1',
  default_options : ['warning_level=0','default_library=static', 'c_std=gnu11', 'buildtype=release'])

# git version
git = find_program('git', required : true)

git_result = run_command([git, 'describe', '--abbrev=4', '--dirty', '--always', '--tags'], capture : true, check : true)
describe = git_result.stdout().strip()

version_c = configure_file(
  input : 'GL/version.c.in',
  output : 'version.c',
  configuration : {'GLDC_VERSION' : describe}
)

# Compiler options
arg_base = ['-fsingle-precision-constant', '-O2', '-g0',
            '-fomit-frame-pointer', '-finline-functions',  '-mfsrra', '-mfsca', '-ml', '-m4-single-only']
#arg_base = ['-fsingle-precision-constant', '-O0', '-g', '-fno-omit-frame-pointer', '-ml', '-m4-single-only']
arg_warnings = ['-Wall', '-Wextra', '-Wformat=0', '-Wno-missing-field-initializers' ]
arg_code = ['-DBUILD_LIBGL', '-DBACKEND_KOSPVR', '-D__DREAMCAST__', '-DFRAME_POINTERS']

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

arg_c = [ arg_base, arg_code, arg_warnings ]
arg_linker = [ ]

inc_base = include_directories('include')
inc_kos = include_directories('/opt/toolchains/dc/kos/include', '/opt/toolchains/dc/kos/kernel/arch/dreamcast/include', '/opt/toolchains/dc/kos/addons/include')

src_library = ['containers/aligned_vector.c', 'containers/named_array.c', 'containers/stack.c', 'GL/draw.c', 'GL/draw.c', 'GL/error.c', 'GL/flush.c', 'GL/fog.c', 'GL/framebuffer.c', 'GL/glu.c', 'GL/immediate.c', 'GL/lighting.c', 'GL/matrix.c', 'GL/state.c', 'GL/texture.c', 'GL/util.c']
src_platform = ['GL/platforms/sh4.c']
src_allocator = ['GL/alloc/alloc.c']

src = [ src_library, src_platform, src_allocator]
inc = [ inc_base, inc_kos ]

# GLdc library and dep
libgldc = static_library('GLdc', sources: [src, version_c] , include_directories : inc,
  c_args : arg_c, link_args : arg_linker,
  dependencies : [m_dep],
  install : true)

libgldc_dep = declare_dependency(include_directories : inc, link_with : libgldc)

