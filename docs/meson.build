if get_option('man')
  man1_dir = get_option('mandir') + '/man1'

  xsltproc = find_program('xsltproc', required : true)
  xsltproc_command = [
    xsltproc,
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '--stringparam', 'man.authors.section.enabled', '0',
    '--stringparam', 'man.copyright.section.enabled', '0',
    '-o', '@OUTPUT@',
    'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
    '@INPUT@',
  ]

  custom_target('fdlinecombine-man',
    input: 'fdlinecombine.xml',
    output: 'fdlinecombine.1',
    command: xsltproc_command,
    install: true,
    install_dir: man1_dir)
endif
