<div id="spacewar" class="insetcontent">

<table width="100%">
  <tr>
  <td>
  <table width="320px">
   <tr width="100%">
       <td width="33%"></td>
       <td width="33%">
            <img id="0-up" src="images/blueup.png" alt="Up">
       </td>
   </tr>

    <tr width="100%">
        <td width="33%">
            <img id="0-left" src="images/blueleft.png" alt="Left">
        </td>
        <td width="33%">
        </td>
        <td width="33%">
            <img id="0-right" src="images/blueright.png" alt="Right">
        </td>
    </tr>

    <tr width="100%">
        <td width="33%">
        </td>
        <td width="33%">
            <img id="0-fire" src="images/attack.png" alt="Fire">
        </td>
        <td width="33%"> </td>
    </tr>
    </table>

</td>


  <td>
   <table width="320px">
   <tr width="100%">
       <td width="33%"></td>
       <td width="33%">
            <img id="1-up" src="images/redup.png" alt="Up">
       </td>
   </tr>

    <tr width="100%">
        <td width="33%">
            <img id="1-left" src="images/redleft.png" alt="Left">
        </td>
        <td width="33%">
        </td>
        <td width="33%">
            <img id="1-right" src="images/redright.png" alt="Right">
        </td>
    </tr>

    <tr width="100%">
        <td width="33%">
        </td>
        <td width="33%">
            <img id="1-fire" src="images/attack.png" alt="Fire">
        </td>
        <td width="33%"> </td>
    </tr>
    </table>

</td>
</tr>
<tr>
<td>
<div title="Start spacewar running" onclick="run_command('spacewar');" class="button green">Start</div>
</td>
<td>
<div title="Stop spacewar " onclick="run_command('stopspacewar');" class="button red">Stop</div>
</td>
</tr>
</table>

    </div>

   <script src="jquery-1.6.4.js"></script>
   <script>
     $(document).ready(function(){
       $("img").click(function(event){
         $.ajax({ url: "writefifo.php?cmd=" + event.target.id});
       });
     });
   </script>



</div>
