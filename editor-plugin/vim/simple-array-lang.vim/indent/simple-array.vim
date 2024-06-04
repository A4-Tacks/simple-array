" Vim syntax file
" Language:		syml
" Maintainer:		A4-Tacks <wdsjxhno1001@163.com>
" Last Change:		2024-06-05
" URL:			https://github.com/A4-Tacks/simple-array

if exists('b:did_indent')
    finish
endif
let b:did_indent = 1

function s:SimpleArrayIndent()
    let preline = prevnonblank(v:lnum-1)
    let ind = indent(preline)

    if getline(preline) =~# '{\s*$'
        let ind += &shiftwidth
    en

    if getline(v:lnum) =~# '^\s*}'
        let ind -= &shiftwidth
    en

    return ind
endfunction

setlocal indentexpr=s:SimpleArrayIndent()
setlocal foldmethod=syntax

" vim:ts=8 sts=8 noet nowrap
