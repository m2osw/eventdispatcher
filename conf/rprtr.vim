" Vim syntax file
" Language:	reporter
" Maintainer:	Alexis Wilke <alexis@m2osw.com>
" Last change:	2024 Mar 17
"
" Installation:
"
" To use this file, add something as follow in your .vimrc file:
"
"   if !exists("my_autocommands_loaded")
"     let my_autocommands_loaded=1
"     au BufNewFile,BufReadPost *.rprtr    so $HOME/vim/rprtr.vim
"   endif
"
" Obviously, you will need to put the correct path to the rprtr.vim
" file before it works, and you may want to use an extension other
" than .rprtr.
"
"
" Copyright (c) 2016-2024  Made to Order Software Corp.  All Rights Reserved
"
" This program is free software: you can redistribute it and/or modify
" it under the terms of the GNU General Public License as published by
" the Free Software Foundation, either version 3 of the License, or
" (at your option) any later version.
"
" This program is distributed in the hope that it will be useful,
" but WITHOUT ANY WARRANTY; without even the implied warranty of
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
" GNU General Public License for more details.
"
" You should have received a copy of the GNU General Public License
" along with this program.  If not, see <https://www.gnu.org/licenses/>.
"


" Remove any other syntax
syn clear


set formatoptions-=tc
set formatoptions+=or

" minimum number of lines for synchronization
" /* ... */ comments can be long
syn sync minlines=1500


" Operators
syn match	rprtrOperator		"+"
syn match	rprtrOperator		"-"
syn match	rprtrOperator		"\*"
syn match	rprtrOperator		"/"
syn match	rprtrOperator		"%"

syn case match

" Internal Instructions
syn keyword	rprtrKeyword		label goto return call

" Constants
syn match	rprtrConstant		"\<0[xX][0-9A-Fa-f]\+n\?\>"
syn match	rprtrConstant		"\<[1-9][0-9]*\.\=[0-9]*\([eE][+-]\=[0-9]\+\)\=\>"
syn match	rprtrConstant		"\<0\=\.[0-9]\+\([eE][+-]\=[0-9]\+\)\=\>"
syn region	rprtrConstant		start=+"+ skip=+\\.+ end=+"+
syn region	rprtrConstant		start=+'+ skip=+\\.+ end=+'+
syn region	rprtrConstant		start=+`+ skip=+\\.+ end=+`+


" Comments
syn keyword	rprtrTodo		contained TODO FIXME XXX
syn match	rprtrTodo		contained "WATCH\(\s\=OUT\)\="
syn region	rprtrLComment		start="//" end="$" contains=as2jsTodo


let b:current_syntax = "rprtr"

if !exists("did_rprtr_syntax_inits")
  let did_rprtr_syntax_inits = 1
  hi link rprtrKeyword			Keyword
  hi link rprtrMComment			Comment
  hi link rprtrLComment			Comment
  hi link rprtrTodo			Todo
  hi link rprtrType			Type
  hi link rprtrOperator			Operator
  hi link rprtrConstant			Constant
  hi link rprtrTemplate			Constant
  hi link rprtrRegularExpression	Constant
  hi      rprtrLabel			guifg=#cc0000
  hi      rprtrGlobal			guifg=#00aa88
endif
