" Vim syntax file
" Language:		syml
" Maintainer:		A4-Tacks <wdsjxhno1001@163.com>
" Last Change:		2024-06-15
" URL:			https://github.com/A4-Tacks/simple-array

if exists("b:current_syntax")
    finish
endif

" syntax define {{{1
syn case match
setlocal iskeyword=33,35-38,40-58,60-122,124,126

syn match	simpleArrayKey /[^"'{}; \t\r\n]\+/
syn match	simpleArrayKey /'[^'\r\n]*'/
syn region	simpleArrayKey start=/"/ end=/"/	contains=simpleArrayStringEscape,simpleArrayStringEscapeErr

syn match	simpleArrayStringEscapeErr /\\./				contained
syn match 	simpleArrayStringEscape	/\\$/					contained
syn match 	simpleArrayStringEscape	/\\[nrtae"\\]/				contained
syn match 	simpleArrayStringEscape	/\\x\%(_*[0-9a-fA-F]\)\{2}/		contained
syn match	simpleArrayStringEscape	/\\x{\%(_*[0-9a-fA-F]\{2}\)*_*}/	contained
syn match	simpleArrayStringEscape	/\\;\@=/				contained nextgroup=simpleArrayComment

syn match	simpleArrayListErr	/}/
syn region	simpleArrayList		start=/{/ end=/}/	fold contains=TOP,simpleArrayListErr

syn match	simpleArrayComment	/;[^\r\n]*\r\=$/

" link colors {{{1
hi def link simpleArrayKey			Keyword
hi def link simpleArrayStringHexErr		Error
hi def link simpleArrayStringEscapeErr		Error
hi def link simpleArrayStringEscape		Special
hi def link simpleArrayComment			Comment
hi def link simpleArrayList			NONE
hi def link simpleArrayListErr			Error
" }}}1

let b:current_syntax = 'simple-array'
" vim:ts=8 sts=8 noet nowrap
