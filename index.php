<html>
<head>
<title>Hack Factory LED Light control server</title>
</head>
<h1>Hack Factory LED Light control server</h1>
<body>
<?php

require_once 'HTML/QuickForm2.php';
$form = new HTML_QuickForm2('LEDControl');

$init =    $form->addElement('submit', 'init', array('value' => 'Init'));
$clear =   $form->addElement('submit', 'clear', array('value' => 'Clear'));
$chase =   $form->addElement('submit', 'chase', array('value' => 'Chase'));
$reset =   $form->addElement('submit', 'reset', array('value' => 'Reset'));
$warstat = `ps auxwwwwww | grep war | grep -v grep`;
if (isset($warstat) && strlen($warstat) > 0)
    $war =   $form->addElement('submit', 'war', array('value' => 'Stop War'));
else
    $war =   $form->addElement('submit', 'war', array('value' => 'Start War'));

/*
$str =    $form->addElement('text', 'str', array('size' => 5, 'maxlength' => 5));
$str->setLabel('Enter String Number, or range separated by dash');

$addr =    $form->addElement('text', 'addr', array('size' => 5, 'maxlength' => 5));
$addr->setLabel('Enter Bulb Address, or range separated by dash');

$bright =    $form->addElement('text', 'bright', array('size' => 3, 'maxlength' => 3));
$bright->setLabel('Enter Brightness, max 204');

$red =    $form->addElement('text', 'red', array('size' => 5, 'maxlength' => 5));
$red ->setLabel('Enter red, or range separated by dash');

$green =    $form->addElement('text', 'green', array('size' => 5, 'maxlength' => 5));
$green ->setLabel('Enter green, or range separated by dash');

$blue =    $form->addElement('text', 'blue', array('size' => 5, 'maxlength' => 5));
$blue ->setLabel('Enter blue, or range separated by dash');

$delay =    $form->addElement('text', 'delay', array('size' => 10, 'maxlength' => 10));
$delay ->setLabel('Enter delay in microseconds between instructions');

$bulb = $form->addElement('submit', 'bulb', array('value' => 'Set lights!'));


*/

$ledscrollstat = `ps auxwwwwww | grep ledscroll | grep -v grep`;

$form->addElement('static', 'mymess', array('content' => 'This is a static message.'));
$display = $form->addElement('submit', 'display', array('value' => 'Toggle Display'));

$message = $form->addElement('text', 'message', array('size' => 50, 'maxlength' => 255));
$change_message = $form->addElement('submit', 'change_message', array('value' => 'Change Message'));
$stop_message = $form->addElement('submit', 'stop_message', array('value' => 'Stop Message'));


if ($stop_message->getValue() == 'Stop Message')
{
    echo "<pre>\n";
    $cmd = "pkill ledscroll";
    echo "$cmd<br>";
    system($cmd);
    echo "</pre><p>...done.</p>";
}

if (strlen($message->getValue()) > 0 && $change_message->getValue() == 'Change Message')
{
    echo "<p>Changing Message to " . $message->getValue() . "</p>";
    echo "<pre>\n";
    $cmd = "pkill ledscroll";
    echo "$cmd<br>";
    system($cmd);
    $cmd = "nohup ./ledscroll elegante_pixel.ttf '" . $message->getValue() . "' >/dev/null 2>&1 &";
    echo "<br>$cmd : <br>";
    system($cmd);
    echo "</pre><p>...done.</p>";
}

$cmd = "./drive ";
$description = NULL;

if (isset($war) && strlen($war ->getValue()) > 0)
    if (isset($warstat) && strlen($warstat) > 0)
    {
        $description = "Stopping war program and ledscroll er";
        $cmd = "pkill war ; pkill ledscroll";
    }
    else
    {
        $description = "Starting war program...";
        $cmd = "nohup ./war >/dev/null 2>&1 &";
    }


if (isset($str) && strlen($str->getValue()) > 0)
    $cmd .= "--string=" . $str->getValue() . " ";

if (isset($addr) && strlen($addr->getValue()) > 0)
    $cmd .= "--addr=" . $addr->getValue() . " ";

if (isset($bright) && strlen($bright->getValue()) > 0)
    $cmd .= "--bright=" . $bright->getValue() . " ";

if (isset($red) && strlen($red->getValue()) > 0)
    $cmd .= "--red=" . $red->getValue() . " ";

if (isset($green) && strlen($green->getValue()) > 0)
    $cmd .= "--green=" . $green->getValue() . " ";

if (isset($blue) && strlen($blue->getValue()) > 0)
    $cmd .= "--blue=" . $blue->getValue() . " ";

if (isset($delay) && strlen($delay->getValue()) > 0)
    $cmd .= "--delay=" . $delay->getValue() . " ";


if ($init->getValue() == "Init")
{
    $description = "Initializing LEDs";
    $cmd .= "init";
}

else if ($clear->getValue() == "Clear")
{
    $description = "Turnning off LEDs";
    $cmd .= "clear";
}
else if ($chase->getValue() == "Chase")
{
    $description = "Running chase from 0-49 across LEDs";
    $cmd .= "chase";
}
else if ($reset->getValue() == "Reset")
{
    $description = "Running make reset";
    $cmd = "mkdir /tmp/www-hack 2>&1";
    echo $cmd . ":\n";
    system($cmd);
    $cmd = "OUTDIR=/tmp/www-hack make reset 2>&1";
}
else if (isset($display) && $display->getValue() == "Toggle Display")
{
    $description = "Toggle display of message";
    $cmd .= "display";
}

else if (isset($bulb) && $bulb->getValue() == "Set lights!")
{
    $description = "Set the lights on";
    $cmd .= "bulb";
}




if ($description)
{
    echo "<p>" . $description . "...<br>";
    echo "<pre>\n";
    echo $cmd . ":\n";
    system($cmd);
    echo "</pre><p>...done.</p>";
}
else
{
    echo "<h2>\n";
    system("./drive status");
    echo "</h2>\n";
}

$form2 = new HTML_QuickForm2('LEDControl');

/*
$rc = $form2->validate(); 
if ($rc)
    echo "<p> true</p>";
else
    echo "<p> false</p>";

{
    echo "<p>validate returns ";
    print_r($rc);
    echo "</p>";
//echo '<h1>Hello, ' . htmlspecialchars($name->getValue()) . '!</h1>';
//exit;
}

*/

// Output the form
echo $form;
echo $form2;

?>
</body>
</html>
