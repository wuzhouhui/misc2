"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" this vimrc was modifyed by me under the basics of others.	"
" if error occured when vim init about 'ctags', it is		"
" because of your system has no ctags. install it and the	"
" error would gone.						"
" wirtten by 1530108435@qq.com, Thu Feb  5 18:13:12 CST 2015	"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" default file encoding
set fenc=utf-8
set fencs=utf-8,gbk,usc-bom,euc-jp,big5,gb18030,gb2312,cp936

" do not use keyboard pattern of vi
set nocompatible

"colorscheme pablo

" highligh trailing space or tab
"match errorMsg /\t$\| $/

" the length of history
set history=1000

" auto reload file when file modifyed by others
set autoread

" using space to perform page down
nmap <Space> <C-f>
" using backspace to perform page up
nmap <BS> <C-b>

" set filetype to markdown for *.md
autocmd bufNewFile,BufReadPost *.md set filetype=markdown

" automatically removing all trailing whitespace when saving files
"autocmd BufWritePre * :%s/\s\+$//e

" auto reload vimrc after modified it(but it seems not worked
" some time)
autocmd! bufwritepost vimrc source ~/.vimrc

" do not redraw screen when execute macro
set nolazyredraw

" highligh current line
"set cul

" do not inherit previous line comment(but it seems not
" workd some time)
"set fo-=r
"set fo-=o

" when file is read only or not modifed but not saved,
" pop confirm prompt
set confirm

" detect file type
filetype on

" turn off preview windows when smart autocomplete
set completeopt=longest,menu

" load file plugin
filetype plugin on

" load file indent file
filetype indent on

" width of line, doesn't work for code
set textwidth=80

" enable tools for cmake or doxygen, cvim plugin requested
let g:C_UseTool_cmake = 'yes'
let g:C_UseTool_doxygen = 'yes'

" hide scroll bar for gvim
"set guioptions-=b
"set guioptions-=R
"set guioptions-=r
"set guioptions-=l
"set guioptions-=L

" hide menu and tool bar for gvim
"set guioptions-=m
"set guioptions-=T

" do not split word that include follow char
set iskeyword+=_,$,@,%,#,-

" highlight syntax
syntax on

" do not backup file
set nobackup

" generate swap file and delete it when delete buffer
set noswapfile
set bufhidden=hide

" pixels bwtween char
set linespace=0

" cmd auto comlete operation in enhanced pattern
set wildmenu

" the height of cmd line
set cmdheight=1

" make backspace work for indent, eol, etc
set backspace=2

" allow backspace and cursor span bound of line
set whichwrap+=<,>,h,l

" do not prompt of uganda
set shortmess=atI

" do not bells
set noerrorbells

" show blank bwtween windows splited
set fillchars=vert:\ ,stl:\ ,stlnc:\

" highlight brackets that matched
set showmatch

" brackets include < and >
set mps+=<:>

" ignore case when searching
set ignorecase

" do not highlight word matched to search
set nohlsearch

" set bg color word matched searching
hi Search ctermbg=white ctermfg=black

" show where the pattern when typing a search cmd
set incsearch

" minimal number of screen lines to keep above
" and below the cursor
set scrolloff=3

" do not use visual bell
set novisualbell

" content of status line
set statusline=%t%m%r%h%w\ (%{&ff}){%Y}[%l,%v][%p%%]\ %{strftime(\"20%y-%m-%d\ %H:%M\")}

" the location of status line
set laststatus=2

" color of status line
hi StatusLine ctermbg=white ctermfg=black

" inherit previos line's indent
set autoindent

" do smart autoindent when starting a new line
set smartindent

" use the C indenting rule
set cindent

" 'switch' align with 'case'
set cino=:0

" always show tab line
set showtabline=2

" show line number
set nonumber

" width of tab
set ts=8

" number of spaces to use for each step of indent. used
" for >>, <<, etc. same as 'tabstop' generally
set sw=8

" width of soft tab
set sts=8

" do not use space replace tab
set noexpandtab

" not wrap
set wrap

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" 		settings about ctags			"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" sort by name
let Tlist_Sort_Type = "name"

" only show one file
let Tlist_Show_One_File=1

" exit program when exit last window
let Tlist_Exit_OnlyWindow=1

" the dir of ctags executable
let Tlist_Ctags_Cmd="/usr/bin/ctags-exuberant"

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
"		settings about cscope			"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
"set cscopequickfix=s-,c-,d-,i-,t-,e-

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" 		settings about latex			"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" IMPORTANT: grep will sometimes skip displaying the file name if you
" search in a singe file. This will confuse Latex-Suite. Set your grep
" program to always generate a file-name.
"set grepprg=grep\ -nH\ $*

" OPTIONAL: Starting with Vim 7, the filetype of empty .tex files defaults to
" 'plaintex' instead of 'tex', which results in vim-latex not being loaded.
" The following changes the default filetype back to 'tex':
"let g:tex_flavor='latex'

" if the file opened is *.tex, reset ts, sw and sts
"if has("autocmd")
	"autocmd FileType tex,java,text set ts=4 sts=4 sw=4 et
"endif

let g:no_atp=1
let g:Tex_SmartKeyQuote=0
let g:Tex_AutoFolding=0
let g:Imap_UsePlaceHolders=0

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" 		   Autocommands				"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" compile C
imap <F5> <esc>:call CompileRunGcc()<CR>
vmap <F5> <esc>:call CompileRunGcc()<CR>
nmap <F5> :call CompileRunGcc()<CR>
func! CompileRunGcc()
	exec "w"
	exec "!gcc % -g"
	"exec "!gcc % -o %<"
	"exec "! ./%<"
endfunc

" compile tex
imap <F6> <esc>:call CompileTex()<CR>
vmap <F6> <esc>:call CompileTex()<CR>
nmap <F6> :call CompileTex()<CR>
func! CompileTex()
	exec "w"
	exec "!xelatex %"
endfunc

" compile java
" -g: with debugging info
imap <F7> <esc>:call CompileRunJavac()<CR>
vmap <F7> <esc>:call CompileRunJavac()<CR>
nmap <F7> :call CompileRunJavac()<CR>
func! CompileRunJavac()
	exec "w"
	exec "!javac -g %"
endfunc

" F2 to save
imap <F2> <esc>:w<CR>:echo expand("%f") . " saved."<CR>
vmap <F2> <esc>:w<CR>:echo expand("%f") . " saved."<CR>
nmap <F2> :w<CR>:echo expand("%f") . " saved."<CR>

"in visual mode when you press * or # to search
"for the current selection, copied from others
vnoremap <silent>* :call VisualSearch('f')<CR>
vnoremap <silent># :call VisualSearch('b')<CR>
function! VisualSearch(direction) range
	let l:saved_reg = @"
	execute "normal! vgvy"
	let l:pattern = escape(@", '\\/.*$^~[]')
	let l:pattern = substitute(l:pattern, "\n$", "", "")
	if a:direction == 'b'
		execute "normal ?" . l:pattern . ""
	elseif a:direction == 'f'
		execute "normal /" . l:pattern .""
	endif
	let @/ = l:pattern
	let @" = l:saved_reg
endfunction

" toggle folding using space
set foldenable
set foldmethod=manual
"nnoremap <space> @=((foldclosed(line('.')) < 0) ? 'zc' : 'zo')<CR>

" save folding info when exit, and reload folding info when init
"autocmd BufWinLeave *.* mkview
"autocmd BufWinEnter *.* silent loadview

" show number of tab page, copied from others
set tabline=%!MyTabLine()  " custom tab pages line
function! MyTabLine()
	let s = '' " complete tabline goes here
	" loop through each tab page
	for t in range(tabpagenr('$'))
		" set highlight
		if t + 1 == tabpagenr()
			let s .= '%#TabLineSel#'
		else
			let s .= '%#TabLine#'
		endif
		" set the tab page number (for mouse clicks)
		let s .= '%' . (t + 1) . 'T'
		let s .= ' '
		" set page number string
		let s .= t + 1 . ' '
		" get buffer names and statuses
		let n = ''	"temp string for buffer names while we loop and check buftype
		let m = 0	" &modified counter
		let bc = len(tabpagebuflist(t + 1))	"counter to avoid last ' '
		" loop through each buffer in a tab
		for b in tabpagebuflist(t + 1)
			" buffer types: quickfix gets a [Q], help gets [H]{base fname}
			" others get 1dir/2dir/3dir/fname shortened to 1/2/3/fname
			if getbufvar( b, "&buftype" ) == 'help'
				let n .= '[H]' . fnamemodify( bufname(b), ':t:s/.txt$//' )
			elseif getbufvar( b, "&buftype" ) == 'quickfix'
				let n .= '[Q]'
			else
				let n .= pathshorten(bufname(b))
			endif
			" check and ++ tab's &modified count
			if getbufvar( b, "&modified" )
				let m += 1
			endif
			" no final ' ' added...formatting looks better done later
			if bc > 1
				let n .= ' '
			endif
			let bc -= 1
		endfor
		" add modified label [n+] where n pages in tab are modified
		if m > 0
			let s .= '[' . m . '+]'
		endif
		" select the highlighting for the buffer names
		" my default highlighting only underlines the active tab
		" buffer names.
		if t + 1 == tabpagenr()
			let s .= '%#TabLineSel#'
		else
			let s .= '%#TabLine#'
		endif
		" add buffer names
		if n == ''
			let s.= '[New]'
		else
			let s .= n
		endif
		" switch to no underlining and add final space to buffer list
		let s .= ' '
	endfor
	" after the last tab fill with TabLineFill and reset tab page nr
	let s .= '%#TabLineFill#%T'
	" right-align the label to close the current tab page
	if tabpagenr('$') > 1
		let s .= '%=%#TabLineFill#%999Xclose'
	endif
	return s
endfunction

" update tags file automatically after saving souce file
" Increment the number below for a dynamic #include guard
let s:autotag_vim_version=1

if exists("g:autotag_vim_version_sourced")
   if s:autotag_vim_version == g:autotag_vim_version_sourced
      finish
   endif
endif

let g:autotag_vim_version_sourced=s:autotag_vim_version

" This file supplies automatic tag regeneration when saving files
" There's a problem with ctags when run with -a (append)
" ctags doesn't remove entries for the supplied source file that no longer exist
" so this script (implemented in python) finds a tags file for the file vim has
" just saved, removes all entries for that source file and *then* runs ctags -a

if has("python")
python << EEOOFF
import os
import string
import os.path
import fileinput
import sys
import vim
import time
import logging
from collections import defaultdict

# global vim config variables used (all are g:autotag<name>):
# name purpose
# maxTagsFileSize a cap on what size tag file to strip etc
# ExcludeSuffixes suffixes to not ctags on
# VerbosityLevel logging verbosity (as in Python logging module)
# CtagsCmd name of ctags command
# TagsFile name of tags file to look for
# Disabled Disable autotag (enable by setting to any non-blank value)
# StopAt stop looking for a tags file (and make one) at this directory (defaults to $HOME)
vim_global_defaults = dict(maxTagsFileSize = 1024*1024*7,
                           ExcludeSuffixes = "tml.xml.text.txt",
                           VerbosityLevel = logging.WARNING,
                           CtagsCmd = "ctags",
                           TagsFile = "tags",
                           Disabled = 0,
                           StopAt = 0)

# Just in case the ViM build you're using doesn't have subprocess
if sys.version < '2.4':
   def do_cmd(cmd, cwd):
      old_cwd=os.getcwd()
      os.chdir(cwd)
      (ch_in, ch_out) = os.popen2(cmd)
      for line in ch_out:
         pass
      os.chdir(old_cwd)

   import traceback
   def format_exc():
      return ''.join(traceback.format_exception(*list(sys.exc_info())))

else:
   import subprocess
   def do_cmd(cmd, cwd):
      p = subprocess.Popen(cmd, shell=True, stdout=None, stderr=None, cwd=cwd)

   from traceback import format_exc

def vim_global(name, kind = string):
   ret = vim_global_defaults.get(name, None)
   try:
      v = "g:autotag%s" % name
      exists = (vim.eval("exists('%s')" % v) == "1")
      if exists:
         ret = vim.eval(v)
      else:
         if isinstance(ret, int):
            vim.command("let %s=%s" % (v, ret))
         else:
            vim.command("let %s=\"%s\"" % (v, ret))
   finally:
      if kind == bool:
         ret = (ret not in [0, "0"])
      elif kind == int:
         ret = int(ret)
      elif kind == string:
         pass
      return ret

class VimAppendHandler(logging.Handler):
   def __init__(self, name):
      logging.Handler.__init__(self)
      self.__name = name
      self.__formatter = logging.Formatter()

   def __findBuffer(self):
      for b in vim.buffers:
         if b and b.name and b.name.endswith(self.__name):
            return b

   def emit(self, record):
      b = self.__findBuffer()
      if b:
         b.append(self.__formatter.format(record))

def makeAndAddHandler(logger, name):
   ret = VimAppendHandler(name)
   logger.addHandler(ret)
   return ret


class AutoTag:
   MAXTAGSFILESIZE = long(vim_global("maxTagsFileSize"))
   DEBUG_NAME = "autotag_debug"
   LOGGER = logging.getLogger(DEBUG_NAME)
   HANDLER = makeAndAddHandler(LOGGER, DEBUG_NAME)

   @staticmethod
   def setVerbosity():
      try:
         level = int(vim_global("VerbosityLevel"))
      except:
         level = vim_global_defaults["VerbosityLevel"]
      AutoTag.LOGGER.setLevel(level)

   def __init__(self):
      self.tags = defaultdict(list)
      self.excludesuffix = [ "." + s for s in vim_global("ExcludeSuffixes").split(".") ]
      AutoTag.setVerbosity()
      self.sep_used_by_ctags = '/'
      self.ctags_cmd = vim_global("CtagsCmd")
      self.tags_file = str(vim_global("TagsFile"))
      self.count = 0
      self.stop_at = vim_global("StopAt")

   def findTagFile(self, source):
      AutoTag.LOGGER.info('source = "%s"', source)
      ( drive, file ) = os.path.splitdrive(source)
      ret = None
      while file:
         file = os.path.dirname(file)
         AutoTag.LOGGER.info('drive = "%s", file = "%s"', drive, file)
         tagsDir = os.path.join(drive, file)
         tagsFile = os.path.join(tagsDir, self.tags_file)
         AutoTag.LOGGER.info('tagsFile "%s"', tagsFile)
         if os.path.isfile(tagsFile):
            st = os.stat(tagsFile)
            if st:
               size = getattr(st, 'st_size', None)
               if size is None:
                  AutoTag.LOGGER.warn("Could not stat tags file %s", tagsFile)
                  break
               if size > AutoTag.MAXTAGSFILESIZE:
                  AutoTag.LOGGER.info("Ignoring too big tags file %s", tagsFile)
                  break
            ret = (file, tagsFile)
            break
         elif tagsDir and tagsDir == self.stop_at:
            AutoTag.LOGGER.info("Reached %s. Making one %s" % (self.stop_at, tagsFile))
            open(tagsFile, 'wb').close()
            ret = (file, tagsFile)
            break
         elif not file or file == os.sep or file == "//" or file == "\\\\":
            AutoTag.LOGGER.info('bail (file = "%s")' % (file, ))
            break
      return ret

   def addSource(self, source):
      if not source:
         AutoTag.LOGGER.warn('No source')
         return
      if os.path.basename(source) == self.tags_file:
         AutoTag.LOGGER.info("Ignoring tags file %s", self.tags_file)
         return
      (base, suff) = os.path.splitext(source)
      if suff in self.excludesuffix:
         AutoTag.LOGGER.info("Ignoring excluded suffix %s for file %s", source, suff)
         return
      found = self.findTagFile(source)
      if found:
         tagsDir, tagsFile = found
         relativeSource = source[len(tagsDir):]
         if relativeSource[0] == os.sep:
            relativeSource = relativeSource[1:]
         if os.sep != self.sep_used_by_ctags:
            relativeSource = string.replace(relativeSource, os.sep, self.sep_used_by_ctags)
         self.tags[(tagsDir, tagsFile)].append(relativeSource)

   def goodTag(self, line, excluded):
      if line[0] == '!':
         return True
      else:
         f = string.split(line, '\t')
         AutoTag.LOGGER.log(1, "read tags line:%s", str(f))
         if len(f) > 3 and f[1] not in excluded:
            return True
      return False

   def stripTags(self, tagsFile, sources):
      AutoTag.LOGGER.info("Stripping tags for %s from tags file %s", ",".join(sources), tagsFile)
      backup = ".SAFE"
      input = fileinput.FileInput(files=tagsFile, inplace=True, backup=backup)
      try:
         for l in input:
            l = l.strip()
            if self.goodTag(l, sources):
               print l
      finally:
         input.close()
         try:
            os.unlink(tagsFile + backup)
         except StandardError:
            pass

   def updateTagsFile(self, tagsDir, tagsFile, sources):
      self.stripTags(tagsFile, sources)
      if self.tags_file:
         cmd = "%s -f %s -a " % (self.ctags_cmd, self.tags_file)
      else:
         cmd = "%s -a " % (self.ctags_cmd,)
      for source in sources:
         if os.path.isfile(os.path.join(tagsDir, source)):
            cmd += " '%s'" % source
      AutoTag.LOGGER.log(1, "%s: %s", tagsDir, cmd)
      do_cmd(cmd, tagsDir)

   def rebuildTagFiles(self):
      for ((tagsDir, tagsFile), sources) in self.tags.items():
         self.updateTagsFile(tagsDir, tagsFile, sources)
EEOOFF

function! AutoTag()
python << EEOOFF
try:
   if not vim_global("Disabled", bool):
      at = AutoTag()
      at.addSource(vim.eval("expand(\"%:p\")"))
      at.rebuildTagFiles()
except:
   logging.warning(format_exc())
EEOOFF
   if exists(":TlistUpdate")
      TlistUpdate
   endif
endfunction

function! AutoTagDebug()
   new
   file autotag_debug
   setlocal buftype=nowrite
   setlocal bufhidden=delete
   setlocal noswapfile
   normal
endfunction

augroup autotag
   au!
   autocmd BufWritePost,FileWritePost * call AutoTag ()
augroup END

endif " has("python")
