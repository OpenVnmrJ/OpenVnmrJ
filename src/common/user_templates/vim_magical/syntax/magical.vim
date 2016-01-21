" Vim syntax file
" Language:	Magical II
" Maintainer:	Frits Vosman 
" Last Change:	2008 Nov 25

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Magical II is not case sensitive"
syn case ignore

" A bunch of useful C keywords
syn keyword	magStatement	abort abortoff break mod return null
syn keyword	magStatement	size sqrt trunc typeof 
syn keyword	magConditional	if else elseif endif then not and or 
syn keyword	magRepeat	while repeat until endwhile do

syn keyword	magTodo		contained TODO FIXME XXX

" cCommentGroup allows adding matches for special things in comments
syn cluster	magCommentGroup	contains=magTodo

" String and Character constants
" Highlight special characters (those which have a backslash) differently
syn match	magSpecial	display contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"
syn region	magString		start=+L\='+ skip=+\\\\\|\\'+ end=+'+ contains=magSpecial,@Spell
syn region	magString		start=+L\=`+ skip=+\\\\\|\\`+ end=+`+ contains=magSpecial,@Spell

syn match	magCharacter	"L\='[^\\]'"
syn match	magCharacter	"L'[^']*'" contains=magSpecial
syn match	magSpecialError	"L\='\\[^'\"?\\abefnrtv]'"
syn match	magSpecialCharacter "L\='\\['\"?\\abefnrtv]'"
syn match	magSpecialCharacter display "L\='\\\o\{1,3}'"
syn match	magSpecialCharacter display "'\\x\x\{1,2}'"
syn match	magSpecialCharacter display "L'\\x\x\+'"


" catch errors caused by wrong parenthesis and brackets
" also accept <% for {, %> for }, <: for [ and :> for ] (C99)
" But avoid matching <::.
syn cluster	magParenGroup	contains=magParenError,magSpecial,magCommentSkip,magCommentString,magComment2String,@magCommentGroup,magCommentStartError,magUserCont,magBitField,magCommentSkip,magOctalZero,magNumber,magFloat,magOctal,magOctalError,magNumbersCom
if exists("c_no_curly_error")
  syn region	magParen	transparent start='(' end=')' contains=ALLBUT,@magParenGroup,@Spell
  syn match	magParenError	display ")"
  syn match	magErrInParen	display contained "^[{}]\|^<%\|^%>"
elseif exists("c_no_bracket_error")
  syn region	magParen	transparent start='(' end=')' contains=ALLBUT,@cParenGroup,@Spell
  syn match	magParenError	display ")"
  syn match	magErrInParen	display contained "[{}]\|<%\|%>"
else
  syn region	magParen	transparent start='(' end=')' contains=ALLBUT,@magParenGroup,magErrInBracket,@Spell
  syn match	magParenError	display "[\])]"
  syn match	magErrInParen	display contained "[\]{}]\|<%\|%>"
  syn region	magBracket	transparent start='\[\|<::\@!' end=']\|:>' contains=ALLBUT,@magParenGroup,magErrInParen,@Spell
  syn match	magErrInBracket	display contained "[);{}]\|<%\|%>"
endif

"integer number, or floating point number without a dot and with "f".
syn case ignore
syn match	magNumbers	display transparent "\<\d\|\.\d" contains=magNumber,magFloat,magOctalError,magOctal
" Same, but without octal error (for comments)
syn match	magNumbersCom	display contained transparent "\<\d\|\.\d" contains=magNumber,magFloat,magOctal
syn match	magNumber	display contained "\d\+\(u\=l\{0,2}\|ll\=u\)\>"
"hex number
syn match	magNumber	display contained "0x\x\+\(u\=l\{0,2}\|ll\=u\)\>"
" Flag the first zero of an octal number as something special
syn match	magOctal	display contained "0\o\+\(u\=l\{0,2}\|ll\=u\)\>" contains=magOctalZero
syn match	magOctalZero	display contained "\<0"
syn match	magFloat	display contained "\d\+f"
"floating point number, with dot, optional exponent
syn match	magFloat	display contained "\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
"floating point number, starting with a dot, optional exponent
syn match	magFloat	display contained "\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, without dot, with exponent
syn match	magFloat	display contained "\d\+e[-+]\=\d\+[fl]\=\>"
if !exists("c_no_c99")
  "hexadecimal floating point number, optional leading digits, with dot, with exponent
  syn match	magFloat	display contained "0x\x*\.\x\+p[-+]\=\d\+[fl]\=\>"
  "hexadecimal floating point number, with leading digits, optional dot, with exponent
  syn match	magFloat	display contained "0x\x\+\.\=p[-+]\=\d\+[fl]\=\>"
endif

" flag an octal number with wrong digits
syn match	magOctalError	display contained "0\o*[89]\d*"
syn case match

" A comment can contain cString, cCharacter and cNumber.
" But a "*/" inside a cString in a cComment DOES end the comment!  So we
" need to use a special type of cString: cCommentString, which also ends on
" "*/", and sees a "*" at the start of the line as comment again.
" Unfortunately this doesn't very well work for // type of comments :-(
syntax match	magCommentSkip		contained "^\s*\*\($\|\s\+\)"
syntax region	magCommentString	contained start=+L\=\\\@<!"+ skip=+\\\\\|\\"+ end=+"+ end=+\*/+me=s-1 contains=magSpecial,magCommentSkip
syntax region	magComment2String	contained start=+L\=\\\@<!"+ skip=+\\\\\|\\"+ end=+"+ end="$" contains=magSpecial
syntax region 	magCommentL		start="//" skip="\\$" end="$" keepend contains=@magCommentGroup,magComment2String,magCharacter,magNumbersCom,magSpaceError,@Spell,magNumber
syntax region	magComment		matchgroup=magCommentStart start="/\*" end="\*/" contains=@magCommentGroup,magCommentStartError,magCommentString,magCharacter,magNumbersCom,magSpaceError,,magNumber,@Spell fold
syntax region	magComment		matchgroup=magCommentStart start=+\"+ end=+\"+ contains=@magCommentGroup,magCommentStartError,magCommentString,magCharacter,magNumbersCom,magSpaceError,,magNumber,@Spell fold
" keep a // comment separately, it terminates a preproc. conditional
syntax match	magCommentError		display "\*/"
syntax match	magCommentStartError	 display "/\*"me=e-1 contained

" Define {...} as a Block, should this not be done for magical?
syntax region	magBlock	start="{" end="}" transparent fold

" Avoid recognizing most bitfields as labels
syn match	magBitField	display "^\s*\I\i*\s*:\s*[1-9]"me=e-1 contains=magType
syn match	magBitField	display ";\s*\I\i*\s*:\s*[1-9]"me=e-1 contains=magType

if exists("c_minlines")
  let b:c_minlines = c_minlines
else
  if !exists("c_no_if0")
    let b:c_minlines = 50	" #if 0 constructs can be long
  else
    let b:c_minlines = 15	" mostly for () constructs
  endif
endif
exec "syn sync ccomment magComment minlines=" . b:c_minlines

" Define the default highlighting.
" Only used when an item doesn't have highlighting yet
hi def link magFormat		magSpecial
hi def link magCommentL		magComment
hi def link magCommentStart	magComment
hi def link magLabel		Label
hi def link magConditional	Conditional
hi def link magRepeat		Repeat
hi def link magCharacter	Character
hi def link magSpecialCharacter	magSpecial
hi def link magNumber		Number
hi def link magOctal		Number
hi def link magOctalZero	PreProc	 " link this to Error if you want
hi def link magFloat		Float
hi def link magOctalError	magError
hi def link magParenError	magError
hi def link magErrInParen	magError
hi def link magErrInBracket	magError
hi def link magCommentError	magError
hi def link magCommentStartError	magError
hi def link magSpaceError	magError
hi def link magSpecialError	cError
hi def link magOperator		Operator
hi def link magStructure	Structure
hi def link magStorageClass	StorageClass
hi def link magError		Error
hi def link magStatement	Statement
hi def link magType		Type
hi def link magConstant		Constant
hi def link magCommentString	cString
hi def link magComment2String	cString
hi def link magCommentSkip	magComment
hi def link magString		String
hi def link magComment		Comment
hi def link magSpecial		SpecialChar
hi def link magTodo		Todo

let b:current_syntax = "magical"

" vim: ts=8
