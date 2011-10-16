<html>
<head>
<title>Utility to write command to fifo.
</head>
<body>
<?php
if (isset($_GET['cmd']))
{
    $cmd = $_GET['cmd'];
    $handle = fopen("myfifo", "a");
    if (! $handle)
    {
        echo "Hmm.  Got an error.\n<br>";
    }
    else
    {
        fwrite($handle, $cmd . "\n");
        fclose($handle);
        echo $cmd . " issued\n<br>";
    }
}
else
    echo "No cmd provided.\n<br>";
?>
</body>
