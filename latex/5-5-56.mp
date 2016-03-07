% 带有 LaTeX 标签的 MetaPost 文件
% 需要使用 mpost -tex=latex 或 mptopdf --latex 命令编译

verbatimtex
\documentclass{article}
\usepackage{amssymb}
\begin{document} % 后面可以省去 \end{document}
etex

beginfig(1)
	draw fullcicle scaled 2cm;
	label.rt(btex $\Lleftarrow$ unit circle etex, (1cm, 0));
endfig
end.
