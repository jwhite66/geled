<?php
$g_logo = 
"|XXX  X     XX   XX  X  X  X    XXX  XXX X  X XXXXX" .
"|X  X X    X  X X  X X X   X     X  X    X  X   X  " .
"|XXX  X    X  X X    XX    X     X  X XX XXXX   X  " .
"|X  X X    X  X X  X X X   X     X  X  X X  X   X  " .
"|XXX  XXXX  XX   XX  X  X  XXXX XXX  XXX X  X   X  ";

$cmd = 'montage -tile 50x5 -geometry 16x16 ';

foreach (str_split($g_logo) as $c)
{
    if ($c == "|")
    {
        $class = 'class="firstonrow" ';
        continue;
    }
    if ($c == "X")
        $cmd .= ' litblock.png';
    if ($c == " ")
        $cmd .= ' unlitblock.png';
    $class = "";
}
$cmd .= " logo.png\n";
`$cmd`;
?>
