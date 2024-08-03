project('dragonnet', 'c', default_options: ['warning_level=2'])

cc = meson.get_compiler('c')

deps = [
	# dependency('threads'),
	cc.find_library('ws2_32', required: false),
]

include = '.'

dragonnet_lib = library('dragonnet',
	sources: [
		'dragonnet/addr.c',
		'dragonnet/error.c',
		'dragonnet/init.c',
		'dragonnet/listen.c',
		'dragonnet/peer.c',
		'dragonnet/recv.c',
		'dragonnet/recv_thread.c',
		'dragonnet/send.c',
	],
	dependencies: deps,
	include_directories: [
		include,
		'endian.h',
	]
)

dragonnet_dep = declare_dependency(
	link_with: dragonnet_lib,
	dependencies: deps,
	include_directories: include,
)
