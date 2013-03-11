"
" Vim syntax file for the Nim programming language.
" Maintainer: Tom Lee
" Latest Revision: 14 October 2012
"

if exists("b:current_syntax")
    finish
endif

syn keyword nimKeyword nil use ret var and or fn spawn while not _
syn keyword nimKeyword match class this break
syn keyword nimBoolean true false
syn keyword nimConditional if else
syn keyword nimType str int float hash object array error method
syn keyword nimSpecial init recv self range __file__ __line__ compile
syn match nimString '"[^"]*"'
syn region nimString start=+\z(["]\)+ end=+\z1+
syn match nimComment '#.*$'
syn match nimInt '[0-9][0-9]*'
syn match nimFunction '^[a-zA-Z_][a-zA-Z0-9_]*'

let b:current_syntax = "nim"

hi def link nimKeyword Keyword
hi def link nimConditional Conditional
hi def link nimType Type
hi def link nimString String
hi def link nimComment Comment
hi def link nimFunction Function
hi def link nimBoolean Boolean
hi def link nimInt Number
hi def link nimSpecial Identifier

