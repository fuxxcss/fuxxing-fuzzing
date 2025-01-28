# Doc Fuzzer
* [What is it ?](#introduction)
* [How to Install ?](#install)
* [How to Use ?](#fuzz)
* [ToDo](#todo)

## introduction
Doc Formats:
- pdf, epub, mobi, azw3, djvu
- txt, rtf (email), clf (log) 
- doc, xls, ppt, odt, ods, odp
- pdb, dmp, core
- cloud-doc

Targets:
- reader, editor, browser, browser-extension
- web-editor
- printer, email-program, log-program
- core-dump-helper

Such as:
- wechat-review, baidu-netdisk
- ireader, acrobat, foxit, MS-office, wps
- qq-browser, edge, firefox (pdf.js)

## install
``` shell
> git submodule add https://github.com/antlr/antlr4.git antlr4
> git submodule
67228355c5bfd1ed5ebb89e726992ec43dda7b53 antlr4 (4.13.2-12-g67228355c)

```

## fuzz
### antlr4 init
``` shell
> git submodule add -b vx.y.z --depth=1 -- https://github.com/antlr/antlr4.git antlr4
> java 
```

