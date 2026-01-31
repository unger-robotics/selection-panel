% =============================================================================
% techdoc.cls – Technische Dokumentation (XeLaTeX)
% \glqq und \grqq für deutsche Anführungszeichen verwenden
% =============================================================================
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{techdoc}[2026/01/11 Technical Documentation v2.1]

% =============================================================================
% CLASS-OPTIONEN
% =============================================================================
\newif\ifprint      \printfalse
\newif\ifdraft      \draftfalse

\DeclareOption{print}{\printtrue}
\DeclareOption{draft}{\drafttrue}
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions\relax

\LoadClass[a4paper,11pt]{article}

% =============================================================================
% SPRACHE & TYPOGRAFIE
% =============================================================================
\RequirePackage[ngerman]{babel}
\RequirePackage[babel=true]{microtype}
\RequirePackage{csquotes}

% Hurenkinder/Schusterjungen vermeiden
\widowpenalties 3 10000 10000 150
\clubpenalties 3 10000 10000 150
\displaywidowpenalty=10000

% =============================================================================
% FONTS (mit Fallback)
% =============================================================================
\RequirePackage{fontspec}
\defaultfontfeatures{Ligatures=TeX}

% TeX Gyre bevorzugt, Fallback auf Latin Modern
\IfFontExistsTF{TeX Gyre Pagella}{
  \setmainfont{TeX Gyre Pagella}
}{
  \setmainfont{Latin Modern Roman}
}
\IfFontExistsTF{TeX Gyre Heros}{
  \setsansfont{TeX Gyre Heros}
}{
  \setsansfont{Latin Modern Sans}
}
\IfFontExistsTF{TeX Gyre Cursor}{
  \setmonofont[Scale=0.85]{TeX Gyre Cursor}
}{
  \setmonofont[Scale=0.85]{Latin Modern Mono}
}

\RequirePackage{newunicodechar}
\newunicodechar{→}{$\rightarrow$}
\newunicodechar{←}{$\leftarrow$}
\newunicodechar{↔}{$\leftrightarrow$}
\newunicodechar{✓}{$\checkmark$}
\newunicodechar{✗}{$\times$}

% =============================================================================
% MATHE & EINHEITEN
% =============================================================================
\RequirePackage{amsmath,amssymb}
\RequirePackage{mathtools}
\RequirePackage{siunitx}
\sisetup{locale=DE, per-mode=symbol}

% =============================================================================
% FARBEN
% =============================================================================
\RequirePackage[table]{xcolor}

\definecolor{ArduinoTeal}{HTML}{00979D}
\definecolor{ArduinoDark}{HTML}{005C5F}
\definecolor{RaspberryRed}{HTML}{C51A4A}
\definecolor{SuccessGreen}{HTML}{75A928}
\definecolor{WarningOrange}{HTML}{D97706}
\definecolor{MutedText}{HTML}{4A5568}
\definecolor{CodeBg}{HTML}{F8FAFC}
\definecolor{CodeFrame}{HTML}{C8D2DC}

\colorlet{InfoBg}{ArduinoTeal!8}
\colorlet{InfoFrame}{ArduinoTeal}
\colorlet{WarnBg}{WarningOrange!8}
\colorlet{WarnFrame}{WarningOrange}
\colorlet{TipBg}{SuccessGreen!8}
\colorlet{TipFrame}{SuccessGreen}

% =============================================================================
% LAYOUT
% =============================================================================
\RequirePackage{geometry}
\geometry{a4paper, margin=2cm, includeheadfoot}

\RequirePackage{setspace}
\setstretch{1.15}

\RequirePackage{parskip}
\setlength{\parindent}{0pt}

\RequirePackage{enumitem}
\setlist{itemsep=0.2em, topsep=0.4em}

\raggedbottom

% =============================================================================
% ÜBERSCHRIFTEN
% =============================================================================
\RequirePackage{sectsty}
\allsectionsfont{\sffamily\bfseries}
\setcounter{secnumdepth}{3}
\setcounter{tocdepth}{2}

% Paragraph mit Abstand danach (statt run-in)
% In .cls ist @ bereits ein Buchstabe - kein \makeatletter nötig
\renewcommand\paragraph{\@startsection{paragraph}{4}{\z@}%
  {-3.25ex \@plus -1ex \@minus -.2ex}%  % Abstand vorher
  {1.0ex \@plus .2ex}%                   % Abstand nachher (positiv = neue Zeile!)
  {\normalfont\normalsize\sffamily\bfseries}}
\renewcommand\subparagraph{\@startsection{subparagraph}{5}{\z@}%
  {-3.25ex \@plus -1ex \@minus -.2ex}%
  {0.75ex \@plus .2ex}%
  {\normalfont\small\sffamily\bfseries}}

% =============================================================================
% GRAFIK
% =============================================================================
\RequirePackage{graphicx}
\graphicspath{{./}{./images/}}
\RequirePackage{float}

\RequirePackage[
  subrefformat=simple,
  labelformat=simple
]{subcaption}
\renewcommand*{\thesubfigure}{~(\alph{subfigure})}

\RequirePackage{tikz}
\usetikzlibrary{shapes, arrows.meta, positioning, calc}

% =============================================================================
% TABELLEN
% =============================================================================
\RequirePackage{array,booktabs,tabularx}
\RequirePackage{longtable}
\renewcommand{\arraystretch}{1.15}

% =============================================================================
% LISTINGS (Code)
% =============================================================================
\RequirePackage{listings}

\lstdefinestyle{base}{
  basicstyle=\ttfamily\footnotesize,
  keywordstyle=\color{ArduinoDark}\bfseries,
  commentstyle=\color{MutedText}\itshape,
  stringstyle=\color{RaspberryRed},
  frame=single, framesep=4pt,
  rulecolor=\color{CodeFrame},
  backgroundcolor=\color{CodeBg},
  breaklines=false,
  showstringspaces=false,
  xleftmargin=12pt, xrightmargin=4pt,
  aboveskip=1em, belowskip=1em
}

\lstdefinestyle{arduino}{
  style=base, language=C++,
  numbers=left, numberstyle=\tiny\color{MutedText},
  morekeywords={uint8_t,uint16_t,uint32_t,int8_t,int16_t,int32_t,
    bool,byte,String,size_t,
    pinMode,digitalWrite,digitalRead,analogRead,analogWrite,
    delay,delayMicroseconds,millis,micros,
    Serial,SPI,Wire,
    HIGH,LOW,INPUT,OUTPUT,INPUT_PULLUP,
    MSBFIRST,LSBFIRST,SPI_MODE0,SPI_MODE1,SPI_MODE2,SPI_MODE3}
}

\lstdefinestyle{python}{
  style=base, language=Python,
  numbers=left, numberstyle=\tiny\color{MutedText},
  morekeywords={async,await,None,True,False,self,cls,with,as,yield,lambda}
}

\lstdefinestyle{shell}{
  style=base, language=bash, numbers=none,
  morekeywords={sudo,apt,systemctl,journalctl,git,pip,pip3,python3,
    cd,ls,cat,echo,mkdir,cp,mv,rm,chmod,chown,curl,wget,rsync,ssh,scp}
}

\lstdefinestyle{json}{
  style=base, numbers=none,
  morestring=[b]",
  literate=
    *{:}{{{\color{ArduinoDark}:}}}1
    {,}{{{\color{ArduinoDark},}}}1
    {\{}{{{\color{RaspberryRed}\{}}}1
    {\}}{{{\color{RaspberryRed}\}}}}1
    {[}{{{\color{RaspberryRed}[}}}1
    {]}{{{\color{RaspberryRed}]}}}1
}

% Style mit Zeilenumbruch für lange Ausgaben
\lstdefinestyle{wrap}{
  style=base,
  breaklines=true,
  breakatwhitespace=true,
  postbreak=\mbox{\textcolor{MutedText}{$\hookrightarrow$}\space}
}

\lstset{style=arduino}

% =============================================================================
% INFO-BOXEN
% =============================================================================
\RequirePackage{tcolorbox}
\tcbuselibrary{skins,breakable}

\newtcolorbox{infobox}[1][]{
  colback=InfoBg, colframe=InfoFrame,
  fonttitle=\bfseries\sffamily, title={#1},
  boxrule=0.5pt, arc=2pt, breakable,
  left=8pt, right=8pt, top=6pt, bottom=6pt
}

\newtcolorbox{warnbox}[1][]{
  colback=WarnBg, colframe=WarnFrame,
  fonttitle=\bfseries\sffamily, title={#1},
  boxrule=0.5pt, arc=2pt, breakable,
  left=8pt, right=8pt, top=6pt, bottom=6pt
}

\newtcolorbox{tipbox}[1][]{
  colback=TipBg, colframe=TipFrame,
  fonttitle=\bfseries\sffamily, title={#1},
  boxrule=0.5pt, arc=2pt, breakable,
  left=8pt, right=8pt, top=6pt, bottom=6pt
}

% =============================================================================
% CAPTIONS
% =============================================================================
\RequirePackage{caption}
\captionsetup{
  font=small,
  labelfont={bf,sf},
  format=plain,
  justification=justified,
  singlelinecheck=false
}

% =============================================================================
% KOPF- & FUSSZEILEN
% =============================================================================
\RequirePackage{fancyhdr}
\setlength{\headheight}{15pt}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{\sffamily\nouppercase{\leftmark}}
\fancyfoot[C]{\thepage}
\renewcommand{\headrulewidth}{0.4pt}

\fancypagestyle{plain}{
  \fancyhf{}
  \fancyfoot[C]{\thepage}
  \renewcommand{\headrulewidth}{0pt}
}

% =============================================================================
% HYPERREF & REFERENZEN
% =============================================================================
\RequirePackage{hyperref}
\RequirePackage{bookmark}

\ifprint
  \hypersetup{
    hidelinks,
    bookmarksnumbered=true,
    breaklinks=true
  }
\else
  \hypersetup{
    colorlinks=true,
    linkcolor=ArduinoTeal,
    urlcolor=ArduinoTeal,
    citecolor=ArduinoTeal,
    bookmarksnumbered=true,
    breaklinks=true
  }
\fi

% cleveref muss nach hyperref
\RequirePackage[noabbrev, nameinlink, ngerman]{cleveref}

% =============================================================================
% MAKROS
% =============================================================================
\RequirePackage{xspace}

\newcommand{\code}[1]{\texttt{#1}}
\newcommand{\highlight}[1]{\textcolor{ArduinoTeal}{\textbf{#1}}}
\newcommand{\warning}[1]{\textcolor{WarningOrange}{\textbf{#1}}}
\newcommand{\critical}[1]{\textcolor{RaspberryRed}{\textbf{#1}}}

% Abkürzungen mit korrektem Spacing
\newcommand{\zB}{z.\,B.\xspace}
\newcommand{\dhei}{d.\,h.\xspace}  % \dh ist von Babel belegt
\newcommand{\ua}{u.\,a.\xspace}
\newcommand{\vgl}{vgl.\xspace}
\newcommand{\bzw}{bzw.\xspace}
\newcommand{\ggf}{ggf.\xspace}
\newcommand{\evtl}{evtl.\xspace}

% =============================================================================
% DRAFT-MODUS
% =============================================================================
\ifdraft
  \overfullrule=2mm
  \setlength{\marginparwidth}{2cm}  % Für todonotes
  \RequirePackage[textsize=scriptsize, colorinlistoftodos]{todonotes}
\else
  \RequirePackage[disable]{todonotes}
\fi

% =============================================================================
% METADATEN-BEFEHLE
% =============================================================================
\newcommand{\@subtitle}{}
\newcommand{\subtitle}[1]{\renewcommand{\@subtitle}{#1}}

\newcommand{\@tagline}{}
\newcommand{\tagline}[1]{\renewcommand{\@tagline}{#1}}

\newcommand{\@scope}{}
\newcommand{\scope}[1]{\renewcommand{\@scope}{#1}}

\newcommand{\@stack}{}
\newcommand{\stack}[1]{\renewcommand{\@stack}{#1}}

\newcommand{\@version}{1.0.0}
\newcommand{\version}[1]{\renewcommand{\@version}{#1}}

\newcommand{\@credits}{}
\newcommand{\credits}[1]{\renewcommand{\@credits}{#1}}

\newcommand{\@coverimage}{images/cover.pdf}
\newcommand{\coverimage}[1]{\renewcommand{\@coverimage}{#1}}

% Header-Titel setzen
\newcommand{\setheadertitle}[1]{\fancyhead[R]{\sffamily #1}}

% =============================================================================
% TITELSEITE
% =============================================================================
\newcommand{\maketitlepage}{%
  \begin{titlepage}
    \centering
    \vspace*{-0.5cm}

    {\sffamily\bfseries\Huge \@title\par}
    \vspace{0.3cm}
    {\sffamily\Large \@subtitle\par}

    \vspace{0.4cm}
    {\sffamily\itshape \@tagline\par}
    \vspace{0.15cm}
    {\small \@scope\par}

    \vspace{0.5cm}

    \IfFileExists{\@coverimage}{%
      \fbox{\includegraphics[width=0.92\textwidth, height=10cm, keepaspectratio]{\@coverimage}}%
      \vspace{0.5cm}
    }{}

    \centering\sffamily
    \textbf{Keywords}\par\vspace{0.3em}
    {\small \@stack}

    \vfill

    {\footnotesize\sffamily
      \begin{tabular*}{0.92\textwidth}{@{\extracolsep{\fill}} l r}
        Stand: \@date & \@author\\
        Version \@version & Tools: \@credits
      \end{tabular*}
    }
  \end{titlepage}
}

\endinput
