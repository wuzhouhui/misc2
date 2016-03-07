% 带有中文 LaTeX 标签的 MetaPost 文件
% 需要使用 mpost 或 mptopdf 命令编译两遍
input latexmp;
setupLaTeXMP(
		packages="amssymb, CJK",
		preamble=("\AtBeginDocument{\begin{CJK}{UTF8}{gbsn}}" &
			 "\AtEndDocument{\end{CJK}}"));
	 beginfig(1)
	 draw fullcircle scaled 2cm;
	 label.rt(textext("$\Lleftarrow$ 单位圆"), (1cm, 0));
endfig;
end.
