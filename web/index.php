<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<?php
    $g_title = "My test web page";
    include 'common.php';
    include 'headers.php';
?>
    </head>

<script>
     $(document).ready(function(){
         $('.jtextfill').textfill({ maxFontPixels: 70, innerTag: 'a' });
         $.get("status.php", function(data){
             $("#maincontent").html(data);
         });
     });
</script>

    <body>
        <div class="fixedwhole">

            <?php include 'logo.php';?>

              <div class="tabdiv">
                <?php echo menu_html("Status"); ?>
              </div> <!--tabdiv-->

            <div id="maincontent" class="content bg1">
            </div> <!--content -->

        </div> <!--whole div -->

    </body>
</html>
