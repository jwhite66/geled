<script src="ajax-switch/jquery.iphone-switch.js" type="text/javascript"></script>
<div id="dstatus" class="insetcontent">

    <div id="dstatus">
        <div id="spinner_text">Loading</div>
        <img id="spinner" src="loading.gif" alt="Loading">
    </div>

</div>

 <script>
     $.get("webhelper.php?cmd=status", function(data){
         $("#dstatus").html(data);
     });
</script>
