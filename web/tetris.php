   <table width="320px">
   <tr width="100%">
       <td width="33%"></td>
       <td width="33%">
            <img id="rotate" src="rotate64.png" alt="Rotate">
       </td>
   </tr>

    <tr width="100%">
        <td width="33%">
            <img id="left" src="left64.png" alt="Left">
        </td>
        <td width="33%">
        </td>
        <td width="33%">
            <img id="right" src="right64.png" alt="Right">
        </td>
    </tr>

    <tr width="100%">
        <td width="33%">
        </td>
        <td width="33%">
            <img id="drop" src="drop64.png" alt="Fire">
        </td>
        <td width="33%"> </td>
    </tr>
    </table>

   <script src="jquery-1.6.4.js"></script>
   <script>
     $(document).ready(function(){
       $("img").click(function(event){
           send_command("fifo", "&fifo_name=myfifo&fifo_verb=" + event.target.id );
         /*$.ajax({ url: "writefifo.php?cmd=" + event.target.id}); */
       });
     });
   </script>
