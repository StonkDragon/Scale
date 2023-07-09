if exists("b:current_syntax")
  finish
endif

syn keyword scaleKeyword function import end while do done construct final unless if elif elunless then else fi switch case esac default return break continue for foreach in to step decl ref nil true false typeof nameof sizeof typealias as container repeat struct enum interface new is cdecl asm goto label none int float int32 int16 int8 uint32 uint16 uint8 uint int64 uint64 any str self super bool sealed static private export expect unsafe const readonly mut try catch lambda deprecated throw open
syn match scaleKeyword 'c!'
syn match scaleKeyword ':'
syn match scaleKeyword '\.'
syn match scaleKeyword ','
syn match scaleKeyword '+>\?'
syn match scaleKeyword '->\?'
syn match scaleKeyword '*>\?'
syn match scaleKeyword '/>\?'
syn match scaleKeyword '%>\?'
syn match scaleKeyword '|>\?'
syn match scaleKeyword '^>\?'
syn match scaleKeyword '\~>\?'
syn match scaleKeyword '&>\?'
syn match scaleKeyword '@'
syn match scaleKeyword '=\(=\|>\)'
syn match scaleKeyword '!=\?'
syn match scaleKeyword '<\(<\|=\)\?'
syn match scaleKeyword '>\(>\|=\)\?'
syn match scaleKeyword '\['
syn match scaleKeyword '\]'
syn match scaleKeyword '{'
syn match scaleKeyword '}'
syn match scaleKeyword '('
syn match scaleKeyword ')'
syn match scaleKeyword '?'

syn match scaleNumber '-\?\d\+'
syn match scaleNumber '-\?0x\(\d\|[a-f]\|[A-F]\)\+'

syn match scaleNumber '-\?\d\+\.\d\+'
syn match scaleNumber '-\?0x\(\d\|[a-f]\|[A-F]\)\+\.\(\d\|[a-f]\|[A-F]\)\+'

syn match scaleComment "#.*"

syn match scaleIdent '[A-Za-z_]\+\([A-Za-z_0-9]\+\)\?'

syn region scaleString start='\v\"' skip=/\\"/ end='\v\"'

hi def link scaleKeyword Keyword
hi def link scaleNumber Constant
hi def link scaleDesc PreProc
hi def link scaleString PreProc
hi def link scaleComment Comment
