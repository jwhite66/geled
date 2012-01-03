<?php
$g_logo = 
"|XXX  X     XX   XX  X  X  X    XXX  XXX X  X XXXXX" .
"|X  X X    X  X X  X X X   X     X  X    X  X   X  " .
"|XXX  X    X  X X    XX    X     X  X XX XXXX   X  " .
"|X  X X    X  X X  X X X   X     X  X  X X  X   X  " .
"|XXX  XXXX  XX   XX  X  X  XXXX XXX  XXX X  X   X  ";

echo '<!-- Logo generator -->';
echo '<div class="lights">';
foreach (str_split($g_logo) as $c)
{
    if ($c == "|")
    {
        $class = 'class="firstonrow" ';
        continue;
    }
    if ($c == "X")
        echo '<img ' . $class . 'src="litblock.png">';
    if ($c == " ")
        echo '<img ' . $class . 'src="unlitblock.png">';
    $class = "";
}
echo '</div>';
?>
