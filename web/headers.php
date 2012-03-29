<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<html>
  
    <head>
    
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
        <meta http-equiv="Content-Language" content="en" />
        <meta name="author" content="Jeremy White" />
        <meta name="abstract" content="BlockLight - Control TC Maker LED lights." />
        <meta name="description" content="This web site allows control of the LED lights mounted in the glass block windows." />
        <meta name="keywords" content="led block glass block arduino ge color effects" />
        <meta name="distribution" content="global" />
        <meta name="revisit-after" content="1 days" />        
        <meta name="copyright" content="All content (c) Jeremy White - released under the GPL v3" />
        
        <title><?php echo $g_title; ?></title>
        
        <link rel="stylesheet" type="text/css" href="led.css" />
        <link rel="shortcut icon" href="litblock.png"  />
        <link rel="icon" href="litblock.png" />
    
        <script src="jquery-1.6.4.js"></script>
        <script src="jquery-textFill-0.1.js"></script>

       <script type="text/javascript">
            /* Run a command and get the status back */
            function run_command(cmd)
            {
                $.ajax({
                    url: 'webhelper.php?cmd=' + cmd,
                    async : false,
                    success: function( data ) 
                    {
                        $(".statusrow").removeClass("errorcolor");
                        $(".statusrow").addClass("aokcolor");
                        $(".statusrow").html(data);
                    },
                    error: function( data, textStatus, errorThrown ) 
                    {
                        $(".statusrow").removeClass("aokcolor");
                        $(".statusrow").addClass("errorcolor");
                        $(".statusrow").html(errorThrown);
                    }
                  });
            }

            /* Run a command and ignore the result */
            function send_command(cmd, parms)
            {
                $.ajax({
                    url: 'webhelper.php?cmd=' + cmd + parms,
                    async : true
                  });
            }
       </script>

    </head>
