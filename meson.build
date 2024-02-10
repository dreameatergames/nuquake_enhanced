project('nuQuake', 'c', default_options:['c_std=gnu99', 'buildtype=release'])

sources = files(
    'src/client/cl_demo.c',
    'src/client/cl_input.c',
    'src/client/cl_main.c',
    'src/client/cl_parse.c',
    'src/client/cl_tent.c',
    'src/client/keys.c',
    'src/client/menu.c',
    'src/client/snd_dma.c',
    'src/client/snd_mem.c',
    'src/client/view.c',
    'src/common/chase.c',
    'src/common/cmd.c',
    'src/common/common.c',
    'src/common/console.c',
    'src/common/crc.c',
    'src/common/cvar.c',
    'src/common/host.c',
    'src/common/host_cmd.c',
    'src/common/mathlib.c',
    'src/common/net_loop.c',
    'src/common/net_main.c',
    'src/common/sbar.c',
    'src/common/wad.c',
    'src/common/world.c',
    'src/common/zone.c',
    'src/renderer/gl_batcher.c',
    'src/renderer/gl_draw.c',
    'src/renderer/gl_mesh.c',
    'src/renderer/gl_model.c',
    'src/renderer/gl_refrag.c',
    'src/renderer/gl_rlight.c',
    'src/renderer/gl_rmain.c',
    'src/renderer/gl_rmisc.c',
    'src/renderer/gl_rsurf.c',
    'src/renderer/gl_screen.c',
    'src/renderer/gl_warp.c',
    'src/renderer/r_part.c',
    'src/server/sv_main.c',
    'src/server/sv_move.c',
    'src/server/sv_phys.c',
    'src/server/sv_user.c',
    'src/vm/pr_cmds.c',
    'src/vm/pr_edict.c',
    'src/vm/pr_exec.c',
)

host = host_machine.system()
build = get_option('buildtype')

if host == 'windows'
    platform_dir = include_directories('src/win32')
    gl = declare_dependency(link_args : ['-lopengl32', '-lglu32'] )

    add_project_link_arguments('-lwinmm', '-ldxguid', language:'c')

    windres = find_program('x86_64-w64-mingw32-windres')
    win32_rc = custom_target('win32_rc',
                    input: 'src/win32/winrc.rc', 
                    output: 'win32rc.o',
                    command : [windres, '-o', '@OUTPUT@', '--define', 'GCC_WINDRES', '@INPUT@' ],
                )
    platform_extras = [ win32_rc,
    'src/win32/cd_null.c',
    'src/win32/conproc.c',
    'src/win32/in_win.c',
    'src/win32/net_none.c',
    'src/win32/snd_mix.c',
    'src/win32/snd_win.c',
    'src/win32/sys_win.c',
    'src/win32/vid_wgl.c' ]

    if build == 'debug'
        add_global_arguments('-DDEBUG', '-O0', '-g', '-pg', '-finline-small-functions', language: 'c')
        add_project_link_arguments('-pg', language: 'c')
    else
        add_global_arguments('-s', '-O2', '-ffast-math', '-funsafe-math-optimizations', '-funroll-loops', '-DQUIET', language: 'c')
    endif
endif

if host == 'dreamcast'
    platform_dir = include_directories('src/dreamcast', 'deps/libgl/include', 'deps/minilzo',
    '/opt/toolchains/dc/kos/include', '/opt/toolchains/dc/kos/kernel/arch/dreamcast/include', '/opt/toolchains/dc/kos/addons/include', '/opt/toolchains/dc/kos-ports/include')
    
    build_dir = 'build_cdi'
    gl = declare_dependency()

    add_global_arguments('-D_arch_dreamcast', '-D_arch_sub_pristine', '-DBUILD_LIBGL', language:'c')
    
    add_project_link_arguments('-lm', '-lz', '../deps/libgl/libGLdc.a', '-Wl,--start-group', '-lkallisti', '-lc', '-lgcc', '-Wl,--end-group', language:'c')
    
    platform_extras = [
    'src/dreamcast/aica.c',
    'src/dreamcast/cd_kos.c',
    'src/dreamcast/in_dreamcast.c',
    'src/dreamcast/net_none.c',
    'src/dreamcast/menu_dreamcast.c',
    'src/dreamcast/snd_mix_dreamcast.c',
    'src/dreamcast/snddma_dreamcast.c',
    'src/dreamcast/sys_dreamcast.c',
    'src/dreamcast/vid_gldc.c', 
    'src/dreamcast/vmu_misc.c',
    'src/dreamcast/vmu_state.c',
    'src/dreamcast/vmuheader.c', ]

    if build == 'debug'
        add_global_arguments('-DDEBUG', '-O0', '-g', '-finline-small-functions', language: 'c')
        add_project_link_arguments('-pg', language: 'c')
    else
        add_global_arguments('-s', '-O2', '-ffast-math', '-funsafe-math-optimizations', '-funroll-loops', '-DQUIET', language: 'c')
    endif

    #run_target('scramble', command : ['scripts/scramble.sh', build_dir, 'nuQuake' ],  depends: [ nuQuake ])
endif

message('Build type: '+ get_option('buildtype') )

add_global_arguments('-w', '-Wformat=0', '-Wno-missing-field-initializers', '-DGLQUAKE', '-fsingle-precision-constant', language: 'c')

compiler = meson.get_compiler('c')
incdir = include_directories('src', 'src/common', '.')

nuQuake = executable(
	'nuQuake',
	sources,
    platform_extras,
    objects: [ ],
	dependencies: [
	    gl
    ],
	include_directories: [incdir, platform_dir],
    native: false
)

if host == 'dreamcast'
    run_target('scramble', command : ['scripts/scramble.sh', build_dir, 'nuQuake' ],  depends: [ nuQuake ])
endif