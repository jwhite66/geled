<div id="dstatus">
    <div id="spinner_text">Loading</div><img id="spinner" src="loading.gif" alt="Loading">
</div>

 <script>
     $.get("webhelper.php?cmd=status", function(data){
         $("#dstatus").html(data);
     });
</script>
