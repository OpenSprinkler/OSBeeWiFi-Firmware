const char connect_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
</head>
<body>
<caption><b>OSBeeWiFi Config</caption><br><br>
<table cellspacing=4 id='rd'>
<tr><td>(Scanning...)</td></tr>
</table>
<table cellspacing=16>
<tr><td><input type='text' name='ssid' id='ssid'></td><td>(SSID)</td></tr>
<tr><td><input type='password' name='pass' id='pass'></td><td>(Password)</td></tr>
<tr><td><input type='text' name='auth' id='auth'></td><td>(Auth Token)</td></tr>
<tr><td colspan=2><p id='msg'></p></td></tr>
<tr><td><button type='button' id='butt' onclick='sf();' style='height:36px;width:180px'>Submit</button></td><td></td></tr>
</table>
<script>
function id(s) {return document.getElementById(s);}
function sel(i) {id('ssid').value=id('rd'+i).value;}
var tci;
function tryConnect() {
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.ip==0) return;
var ip=''+(jd.ip%256)+'.'+((jd.ip/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)/256>>0);
id('msg').innerHTML='<b><font color=green>Connected! Device IP: '+ip+'</font></b><br>Device is rebooting. Switch back to<br>the above WiFi network, and then<br>click the button below to redirect.'
id('butt').innerHTML='Go to '+ip;id('butt').disabled=false;
id('butt').onclick=function rd(){window.open('http://'+ip);}
clearInterval(tci);
}
}    
xhr.open('POST', 'jt', true); xhr.send();    
}
function sf() {
id('msg').innerHTML='';
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.result==1) { tci=setInterval(tryConnect, 10000); return; }
id('msg').innerHTML='<b><font color=red>Error code: '+jd.result+', item: '+jd.item+'</font></b>';
id('butt').innerHTML='Submit';
id('butt').disabled=false;id('ssid').disabled=false;id('pass').disabled=false;id('auth').disabled=false;
}
}
var comm='cc?ssid='+encodeURIComponent(id('ssid').value)+'&pass='+encodeURIComponent(id('pass').value)+'&auth='+id('auth').value;
xhr.open('POST', comm, true); xhr.send();
id('butt').innerHTML='Connecting...';
id('butt').disabled=true;id('ssid').disabled=true;id('pass').disabled=true;id('auth').disabled=true;
}

function loadSSIDs() {
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText), i;
id('rd').deleteRow(0);
for(i=0;i<jd.ssids.length;i++) {
var cell=id('rd').insertRow(-1).insertCell(0);
cell.innerHTML ="<tr><td><input type='radio' name='ssids' id='rd" + i + "' value='" + jd.ssids[i] + "' onclick=sel(" + i + ")>" + jd.ssids[i] + "</td></tr>";
}
}
};
xhr.open('GET','js',true); xhr.send();
}
setTimeout(loadSSIDs, 1000);
</script>
</body>)";

const char index_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
<script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
</head>
<body>
<div data-role='page' id='page_home'>
<div data-role='header'>
<h3 id='l_name'>OSBWiFi</h3>
<a data-role='button' id='btn_upd' data-theme='a' data-icon='gear' data-iconpos='left' class='ui-btn-right'>Update</a>
</div>
<div role='main' class='ui-content'>
<ul data-role='listview' data-divider-theme='d'>
<li data-role='list-divider'>Status</li>
<li>
<table cellpadding=4 cellspacing=0>
<tr><td colspan=2><span id='l_time'></span> (Ver. <span id='l_fwv'></span>)</td></tr>
<tr><td><b>Valve:</b> <span id='l_sot'></span></td><td><b>WiFi:</b> <span id='l_rssi'></span></td></tr>
</table>
</li>
<li data-role='list-divider'>Zones</li>
<li>
<table cellpadding=4 cellspacing=0>
<script>
function w(s) {document.write(s);}
var i;
for(i=0;i<3;i++) w('<tr><td width=20 align=center><b><span id=zs'+i+' style="color:#800000;font-size:20">&#9898;</span></b></td><td><span id=zon'+i+'>Zone '+(i+1)+'</span></td></tr>');
</script>
</table>
<div id='b_status' style='display:none;'><br>
<table cellpadding=3 cellspacing=0>
<tr align=center><td><img src='http://raysfiles.com/os/sprinkler.gif'></td><td></td></tr>
<tr><td>Running Task <span id='l_tid'>1/1</span></td><td><span id='l_trem'></span></td></tr>
<tr><td>of Program <span id='l_pid'></span></td><td><span id='l_prem'></span></td></tr>
</table>
</div>
</li>
<li data-role='list-divider'>Programs</li>
<li>
<div data-role='controlgroup' data-type='horizontal'>
<script>
var i;
for(i=0;i<6;i++) w('<button onclick=openprog('+i+') id=btn_p'+i+'>'+(i+1)+'</a>');
</script>
</div>
<table><tr><td><button data-theme='b' id='btn_pre'>Program Preview</button></td></tr></table>
</li>
<li data-role='list-divider'>System</li>
<li>
<div data-role='controlgroup' data-type='horizontal'>
<button data-theme='b' id='btn_opt'>Settings</button>
<button data-theme='b' id='btn_man'>Manual</button>  
<button data-theme='b' id='btn_log'>Log</button>
</div>
</li>        
</ul>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler Bee (<a href='http://bee.opensprinkler.com' target='_blank' style='text-decoration:none'>bee.opensprinkler.com</a>)</p>
</div>
<script>
$('#btn_man').click(function(e){window.open('manual.html');});
$('#btn_log').click(function(e){window.open('log.html');});
$('#btn_upd').click(function(e){window.open('update.html');});
$('#btn_opt').click(function(e){window.open('settings.html');});
$('#btn_pre').click(function(e){window.open('preview.html');})
var nprogs=0,ntasks=0,prem=0,trem=0,utct=0,zbits=0,si=0,ti=0;
function openprog(i){
if(i<nprogs) {
window.open('program.html?pid='+i);
} else {
window.open('program.html?pid=-1');
}
}    
$('#page_home').on('pagebeforeshow', function() {
if(!ti) ti=setInterval('updateTimer()', 1000);
updateSystem();
if(!si) si=setInterval('updateSystem()', 3000);
});
function updateTimer() {
if(prem>0) prem--;
if(trem>0) trem--;
if(utct>0) utct++;
showTimer();
}    
function showTimer() {
if (prem==0) {$('#l_prem').text('');}
else if (prem>0){
var h=(prem/3600)>>0;
var m=(prem-h*3600)/60>>0;
var s=(prem%60);
$('#l_prem').text('('+h+':'+(m/10>>0)+(m%10)+':'+(s/10>>0)+(s%10)+')');
}
if (trem==0) {$('#l_trem').text('');}
else if(trem>0){
var h=(trem/3600)>>0;
var m=(trem-h*3600)/60>>0;
var s=(trem%60);
$('#l_trem').text('('+h+':'+(m/10>>0)+(m%10)+':'+(s/10>>0)+(s%10)+')');
}
if (utct==0) {$('#l_time').text('');}
else if(utct>0){
$('#l_time').text((new Date(utct*1000)).toLocaleString());
}
}    
function updateSystem(jd) {
$.getJSON('jc', function(jd) {
utct=jd.utct;
prem=jd.prem;
trem=jd.trem;
nprogs=jd.np;
ntasks=jd.nt;
showTimer();
$('#l_fwv').text(''+(jd.fwv/100>>0)+'.'+(jd.fwv/10%10>>0)+'.'+(jd.fwv%10>>0));
$('#l_sot').text(jd.sot?'NON-latching':'Latching');
$('#l_rssi').text(jd.rssi+' dBm ('+(jd.rssi<-75?'weak':'ok')+')');
$('#l_name').text(jd.name);
zbits = jd.zbits;
for(i=0;i<3;i++) {
if(jd.zons) $('#zon'+i).text(jd.zons[i]);
$('#zs'+i).html(((jd.zbits>>i)&1)?'&#9899;':'&#9898;');
$('#zs'+i).css('color',((jd.zbits>>i)&1)?'#008000':'#800000');
$('#zon'+i).css('font-weight',((jd.zbits>>i)&1)?'bold':'normal');
}
if(jd.pid<0) {$('#b_status').hide();}
else {
$('#l_tid').text(''+(jd.tid+1)+'/'+(ntasks));
$('#l_pid').text(jd.pid>64?String.fromCharCode(jd.pid):(jd.pid+1));
$('#b_status').show();
}
var i,btn;
for(i=0;i<jd.mnp;i++) {
btn=$('#btn_p'+i);
if(i<nprogs) {btn.button('enable');btn.button({theme:'b'});}
else if(i==nprogs) {btn.button('enable');btn.button({theme:'a'});}
else {btn.button('disable');btn.button({theme:'c'});}
}          
});
}
</script>
</div>
</body>)";

const char log_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
<script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
</head>
<body>
<div data-role='page' id='page_log'>
<div data-role='header'><h3 id='l_name'>Log</h3>
<a data-role='button' id='btn_close' data-theme='a' data-icon='delete' data-iconpos='left' class='ui-btn-left'>Close</a>
</div>
<div data-role='content'>
<p>Below are the most recent <label id='l_nr'></label> watering records</p>
<p>Current time is <label id='l_time'></label></p>
<div data-role='fieldcontain'>
<table id='tab_log' border='1' cellpadding='6' style='border-collapse:collapse;'>
<tr><td align='center'><b>Time</b></td><td align='center'><b>Zone</b></td><td align='center'><b>Event</b></td></tr>
</table>
</div>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler Bee (<a href='http://bee.opensprinkler.com' target='_blank' style='text-decoration:none'>bee.opensprinkler.com</a>)</p>
</div>   
<script>
var curr_time=0,ti=0;
var jd;
$('#btn_close').click(function(){close();});
$('#page_log').on('pagebeforeshow', function(){
curr_time = (new Date()).getTime();
show_time();
show_log();
if(!ti) ti=setInterval('show_time()', 1000);
});
function show_time() {
$('#l_time').text((new Date(curr_time)).toLocaleString());
curr_time += 1000;
}
function toHMS(t) {
var h=t/3600>>0;
var m=(t%3600)/60>>0;
var s=t%60;
return ''+(h/10>>0)+(h%10)+':'+(m/10>>0)+(m%10)+':'+(s/10>>0)+(s%10);
}
function show_log() {
$.getJSON('jl', function(jd) {
$('#l_name').text(jd.name);
$('#tab_log').find('tr:gt(0)').remove();
var logs=jd.logs;
logs.sort(function(a,b){return b[0]-a[0];});
$('#l_nr').text(logs.length);
var ldate = new Date();
for(var i=0;i<logs.length;i++) {
var l=logs[i];
ldate.setTime(l[0]*1000);
var r='<tr><td align=center>'+ldate.toLocaleString()+'</td><td>'+jd.zons[l[3]]+'</td><td>';
if(l[2]=='o'.charCodeAt()) {
r+='<b>ON</b> with Prog ';
r+=((l[4]>64)?String.fromCharCode(l[4]):l[4]+1)+', Task ';
r+=(l[5]+1)+'</td></tr>';
}
else if(l[2]=='c'.charCodeAt()) {
r+='Off --> ran for ';
r+=toHMS(l[1]);
r+='</td></tr>';              
}
$('#tab_log').append(r);
}
});
setTimeout(show_log, 15000);
}
</script>
</div>
</body>)";

const char manual_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
<script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
</head>
<body>
<div data-role='page' id='page_manual'>
<div data-role='header'><h3>Manual Control</h3>
<a data-role='button' id='btn_close' data-theme='a' data-icon='delete' data-iconpos='left' class='ui-btn-left'>Close</a>
</div>
<div data-role='content'>
<fieldset data-role='controlgroup' data-type='horizontal'>
<input type='radio' name='rad_progtype' id='testzone' onclick='toggleProgType()' checked><label for='testzone'>Test Zones</label>
<input type='radio' name='rad_progtype' id='quickprog' onclick='toggleProgType()'><label for='quickprog'>Quick Program</label>
</fieldset>
<div id='div_testzone'>
<table><tr><td>Select Zone:</td></tr>
<tr><td>
<script>
function w(s) {document.write(s);}
var i;
for(i=0;i<3;i++) w('<input type=radio name=rad_zone id=z'+i+(i==0?' checked':'')+'><label for=z'+i+'><span id=zon'+i+'>Zone '+(i+1)+'</span></label>');
</script>
</td></tr>
<tr><td><label for='dur'>Test Time (minutes):</label><input type='range' id='dur' value=1 min=1 max=60></td></tr></table>
</div>
<div id='div_quickprog' style='display:none;'>
<table cellpadding=8><tr><td><label for='prog'>Select Program:</lable>
<select name='prog' id='prog'>
<option value=81>Quick Program</option>
<script>
for(i=0;i<6;i++) w('<option value='+i+'>Program '+(i+1)+'</option>');
</script>
</select></td></tr>
<script>
for(i=0;i<3;i++) {
w('<tr><td><label for=dur'+i+'>Zone '+(i+1)+' Duration (minutes):</label><input type=range id=dur'+i+' value=1 min=0 max=60 /></td></tr>');
}
</script>
</table>
</div>
<table><tr><td><b>Device key:</b></td><td><input type='password' size=16 maxlength=32 name='dkey' id='dkey'></td></tr><tr><td colspan=2><label id='msg'></label></td></tr></table>
<div data-role='fieldcontain'>
<div data-role='controlgroup' data-type='horizontal'>
<button data-theme='b' id='btn_submit'>Submit</button>
<button data-theme='a' id='btn_reset'>Reset Zones</button>
<button data-theme='a' id='btn_reboot'>Reboot</button>
</div>
</div>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler Bee (<a href='http://bee.opensprinkler.com' target='_blank' style='text-decoration:none'>bee.opensprinkler.com</a>)</p>
</div>
</div>
<script>
function err_msg(e) {
if(e===16) return 'missing data';
if(e===17) return 'out of bound';
return e;
}
function clear_msg() {$('#msg').text('');}  
function show_msg(s,c) {$('#msg').text(s).css('color',c); setTimeout(clear_msg, 3000);}
function eval_cb(n) {
return $(n).is(':checked')?1:0;
}
function get_zid() {
for(var i=0;i<3;i++) {
if(eval_cb('#z'+i)) return i;
}
}    
function toggleProgType() {
$('#div_testzone').hide();
$('#div_quickprog').hide();
if(eval_cb('#testzone')) $('#div_testzone').show();
if(eval_cb('#quickprog')) $('#div_quickprog').show();
}
$('#btn_close').click(function(){close();});
$(':button').click(function(e){
e.preventDefault();
var id=e.target.id;
var comm;
if(id==='btn_close') {close();return;}
if(id==='btn_submit') {
comm='rp?dkey='+encodeURIComponent($('#dkey').val());
if(eval_cb('#testzone')) {
comm+='&pid='+('T'.charCodeAt());
comm+='&zid='+get_zid();
comm+='&dur='+$('#dur').val()*60;
} else {
comm+='&pid='+$('#prog').val();
if($('#prog').val()=='Q'.charCodeAt()) {
comm+='&durs=[';
for(var i=0;i<3;i++) {
comm+=$('#dur'+i).val()*60;
comm+=(i==2)?']':',';
}
}
}
} else if(id==='btn_reset') {
comm='cc?dkey='+encodeURIComponent($('#dkey').val());        
if(!confirm('Stop all zones?')) return;
else comm+='&reset=1';
} else if(id==='btn_reboot') {
comm='cc?dkey='+encodeURIComponent($('#dkey').val());                
if(!confirm('Reboot controller?')) return;    
else comm+='&reboot=1';
}
$.getJSON(comm, function(jd) {
if(jd.result!=1) {
if(jd.result==2) show_msg('Check device key and try again.','red');
else show_msg('Error: '+err_msg(jd.result)+', item: '+jd.item+'.','red');
} else {
if(id=='btn_reboot') {show_msg('Rebooting...','green');setTimeout(close,2000);}
else {show_msg('Submitted successfully!','green');setTimeout(close,2000);}
}
});
});
$('#prog').bind('change', function(e,ui) {
if($('#prog').val()==('Q'.charCodeAt()))
for(var i=0;i<3;i++)  $('#dur'+i).slider('enable').slider('refresh');
else
for(var i=0;i<3;i++)  $('#dur'+i).slider('disable').slider('refresh');
});
$('#page_manual').on('pagebeforeshow', function() {
$(this).find('.ui-slider').width(300);
});
</script>
</body>)";

const char preview_html[] PROGMEM = R"(<head>
<title>Program Preview</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel="stylesheet" href="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css" />
<script src="http://code.jquery.com/jquery-1.11.1.min.js"></script>
<script src="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js"></script>
</head>
<body>
<div data-role='page' id='page_preview'>
<div data-role='header' data-position='fixed'>
<h3><span id='lbl_date'></span> Preview</h3>
<a href='#' id='btn_close' class='ui-btn ui-btn-icon-left ui-icon-delete'>Close</a>
<div data-role='controlgroup' data-type='horizontal' class='ui-btn-right'>
<a href='#' id='btn_prev' class='ui-btn ui-btn-icon-right ui-icon-arrow-l'>Prev</a>
<a href='#' id='btn_next' class='ui-btn ui-btn-icon-right ui-icon-arrow-r'>Next</a>   
</div>
</div>
<div id='div_plot' data-role='content'>
<script>
var date,tmz=0,intv=15,devday=0,progs;
var xstart=80,ystart=80,stwidth=100,stheight=120,nzones=3;
var winwidth=stwidth*nzones+xstart, winheight=24.5*stheight+ystart;
var prog_color=['rgba(0,0,200,0.3)','rgba(0,200,0,0.3)','rgba(200,0,0,0.3)','rgba(0,200,200,0.3)'];
function loct()   {return date.getTime()+(tmz-48)*900000;}
function getx(zid)  {return xstart+zid*stwidth-15;}  // x coordinate given a zone
function gety(t)    {return ystart+t*stheight/60;}  // y coordinate given a time
function w(s) {document.write(s);}
function plot_bar(zid,start,pid,end) { // plot program bar
$('#div_plot').append('<div class=bar align=center style="position:absolute;background-color:'+prog_color[(pid+3)%4]+';left:'+getx(zid)+';top:'+gety(start/60)+';border:0;width:'+stwidth+';height:'+((end-start)/60*stheight/60)+'">P'+(pid+1)+'</div>');
}
var zid,t;
for(zid=0;zid<=nzones;zid++) {
if(zid<nzones) w('<div style="position:absolute;left:'+(xstart+zid*stwidth+15)+';top:'+(ystart-25)+';width:'+stwidth+';height:20;border:0;padding:0;"><font size=2>Zone '+(zid+1)+'</font></div>');
w('<div style="position:absolute;left:'+getx(zid)+';top:'+(ystart-10)+';border:1px solid gray;width:0;height:'+(winheight-ystart+30)+';"></div>');
}
// horizontal grid, time
for(t=0;t<=24;t++) {
w('<div style="position:absolute;left:'+(xstart-30)+';top:'+gety(t*60)+';border:1px solid gray;width:15;height:0;"></div>');
w('<div style="position:absolute;left:'+(xstart-23)+';top:'+(gety(t*60)+stheight/2)+';border:1px solid gray;width:8;height:0;"></div>');
w('<div style="position:absolute;left:'+(xstart-70)+';top:'+(ystart+t*stheight-7)+';width=70;height:20;border:0;padding:0;"><font size=2>'+(t/10>>0)+(t%10)+':00</font></div>');
}
w('<div id=div_time style="position:absolute;left:'+(xstart-stwidth/2-10)+';top:'+gety(0)+';border:1px solid rgba(200,0,0,0.5);width:'+(winwidth-xstart+stwidth/2)+';height:0;"></div>');   
</script>
</div>
</div>
<script>
$('#btn_close').click(function(){close();});
$('#btn_prev').click(function(){
var t=date.getTime();
if(t>=86400000) date.setTime(t-86400000);
$('#lbl_date').text(date.toLocaleDateString());
$('.bar').remove();
runPrograms();
});
$('#btn_next').click(function(){
date.setTime(date.getTime()+86400000);
$('#lbl_date').text(date.toLocaleDateString());
$('.bar').remove();
runPrograms();
});
$('#page_preview').on('pagebeforeshow', function(){
var ss=window.location.search.split('date=');
if(ss.length==2) {
date=new Date(ss[1]);
}
if((!date) || isNaN(date.getTime())) date=new Date();
$.ajax('/jp', {async:false, dataType:'json', success: function(jdata) {
tmz=jdata.tmz;
progs=jdata.progs;
devday=((new Date().getTime())+(tmz-48)*900000)/86400000>>0;
if(!progs) return;
}});
$('#lbl_date').text(date.toLocaleDateString());
drawTime();
setInterval('drawTime()', intv*1000);
});
$('#page_preview').on('pageshow', function() {
//window.scrollTo(0,100);
runPrograms();
});
function drawTime() {
$('#div_time').css('top',gety(((loct()/1000)%86400)/60));
}
function checkDayMatch(prog,simt) {
var conf=prog.config;
var dayt=(conf>>1)&1,oddeven=(conf>>2)&3;
var days=[(conf>>8)&0xff,(conf>>16)&0xff];
var wd=(date.getDay()+6)%7;
if(dayt==1) { // interval
if (((simt/86400>>0)%days[1]) != ((days[0]+devday)%days[1])) return 0;
} else { // weekly
if (!(days[0] & (1<<wd))) return 0;
}
var dt=date.getDate();
if(oddeven==2) { // even
if(dt%2) return 0;
} else if(oddeven==1) { // odd
if(dt==31 || (dt==29&&date.getMonth()==1) || (dt%2)!=1) return 0;
}
return 1;
}
function checkMatch(prog,simt) {
if(!(prog.config&1)) return 0;
var stt=(prog.config>>4)&3,i;
var curr=(simt%86400)/60>>0;
var sts=prog.sts;
var start=sts[0],repeat=sts[1],interval=sts[2];
if(checkDayMatch(prog,simt)) {
if(stt==0) { return (curr==start)?1:0;}
else if(stt==1) {
for(i=0;i<5;i++) {
if(curr==sts[i]) return 1;
}
} else if(stt==2) {
if(curr==start) return 1;
if (curr>start && interval) {
var c = (curr-start)/interval>>0;
if ((c*interval==(curr-start)) && c<=repeat) {
return 1;
}
}
}
}
if(stt!=2 || !interval) return 0;
if (checkDayMatch(prog,simt-86400)) {
var c = (curr-start+1440)/interval;
if ((c*interval==(curr-start+1440)) && c<=repeat) {
return 1;
}
}
return 0;    
}
function plotProgram(prog,pid,simmin) {
var runtime=0,start=simmin*60,end,dur,zbits;
for(var ti=0;ti<prog.nt;ti++) {
zbits=prog.pt[ti]&0xff;
dur=prog.pt[ti]>>8;
end=start+dur;
for(var zi=0;zi<3;zi++) {
if((zbits>>zi)&1) plot_bar(zi,start,pid,end);
}
start=end;
runtime+=dur;
}
return runtime;
}
function runPrograms() {
var simmin=0,simt,busy,pid,match_found=0;
do {
busy=0;
match_found=0;
for(pid=0;pid<progs.length;pid++) {
var prog=progs[pid];
simt=(loct()/86400000>>0)*86400+simmin*60;
if(checkMatch(prog,simt)) {
match_found=1;
var progress = plotProgram(prog,pid,simmin)/60>>0;
simmin+=(progress>0)?progress:1;
}
}
if(!match_found) simmin++;
} while(simmin<24*60); // simulation ends
}
</script>
</body>)";

const char program_html[] PROGMEM = R"(<head>
<title>Edit Program</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
<script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
<style>
table td, table th { padding: 2; }
</style>
</head>
<body>
<div data-role='page' id='page_prog'>
<div data-role='header'>
<h3>Edit <label id='lbl_pid'></label></h3>
<a data-role='button' id='btn_cancel' data-theme='a' data-icon='delete' data-iconpos='left' class='ui-btn-left'>Cancel</a>
</div>
<div data-role='content'>
<ul data-role='listview' data-divider-theme='d'>
<li>
<table><tr>
<td><b>Name:</b></td><td><input type='text' size=24 maxlength=24 id='name' data-mini='true' value='New Program'></input></td>
<td><input type='checkbox' id='en' data-mini='true' checked><label for='en'>Enabled</label></td>
</tr></table>
</li>
<li data-role='list-divider'>Start Days</li>
<li>
<fieldset data-role='controlgroup' data-mini='true' data-type='horizontal'>
<input type='radio' name='rad_daytype' id='weekly' onclick='toggleDayType()' checked><label for='weekly'>Weekly</label>
<input type='radio' name='rad_daytype' id='interval' onclick='toggleDayType()'><label for='interval'>Interval</label>
</fieldset><br />
<div id='div_weekly'>
<fieldset data-role='controlgroup' data-type='horizontal'>
<script>
function w(s) {document.write(s);}
var s=['M','Tu','W','Th','F','Sa','Su'];
for(i=0;i<7;i++) {
w('<input type=checkbox data-mini=true id=wd'+i+'>');
w('<label for=wd'+i+'>'+s[i]+'</label>');     
}
</script>
</fieldset>
</div>
<div id='div_interval' style='display:none;'>
<table><tr>
<td>Every</td>
<td><input type='text' size=3 maxlength=3 id='intn' value='2' data-mini=true></td><td>days, starting in</td>
<td><input type='text' size=3 maxlength=3 id='intd' value='0' data-mini=true></td><td>day(s).</td>
</tr></table>
</div>
</li>
<li data-role='list-divider'>Restriction</li>
<li>
<fieldset data-role='controlgroup' data-mini='true' data-type='horizontal'>
<input type='radio' name='rad_restrict' id='restr0' checked><label for='restr0'>None</label>
<input type='radio' name='rad_restrict' id='restr1'><label for='restr1'>Odd days</label>
<input type='radio' name='rad_restrict' id='restr2'><label for='restr2'>Even days</label>
</fieldset>
</li>
<li data-role='list-divider'>Start Time</li>
<li>
<table><tr><td>Starts at: </td>
<td><input type=text size=3 maxlength=2 id=st0h value='06' data-mini=true></td><td>:</td>
<td><input type=text size=3 maxlength=2 id=st0m value='00' data-mini=true></td><td>(24-hour clock)</td>
</tr></table>
</li>
<li data-role='list-divider'>Additional Start Times</li>
<li>
<fieldset data-role='controlgroup' data-mini='true' data-type='horizontal'>
<input type='radio' data-mini=true name='rad_sttype' onclick='toggleSTType()' id='stnone' checked><label for='stnone'>None</label>
<input type='radio' data-mini=true name='rad_sttype' onclick='toggleSTType()' id='fixed'><label for='fixed'>Fixed</label>
<input type='radio' data-mini=true name='rad_sttype' onclick='toggleSTType()' id='repeat'><label for='repeat'>Repeat</label>
</fieldset>
<div id='div_fixed' style='display:none;'>
<br />
<span style='font-weight:normal;'>(Up to 4. Leave empty or - to disable.)</span>
<table>
<script>
for(i=1;i<5;i++) {
w('<tr><td><b>'+i+'. </b></td><td><input type=text size=3 maxlength=2 data-mini=true id=st'+i+'h value="-"></td><td>:</td><td><input type=text size=3 maxlength=2 data-mini=true id=st'+i+'m value="-"></td></tr>');
}
</script>
</table>
</div>
<div id='div_repeat' style='display:none;'>
<br />
<table><tr>
<td>Every</td><td><input type='text' size=4 maxlength=4 id='rept' value='60' data-mini=true></td><td>minutes, for</td><td><input type='text' size=5 maxlength=4 id='repk' value='1' data-mini=true></td><td>time(s).</td>
</tr></table>
</div>
</li>
<li data-role='list-divider'>Program Tasks</li>
<li>
<div data-role='fieldcontain'>
<table id='tab_tasks' border=1 cellpadding=4 cellspacing=0 style='border-collapse:collapse;'>
<tr><td><b>Index</b></td><td><b>Zone 1</b></td><td><b>Zone 2</b></td><td><b>Zone 3</b></td><td><b>Duration</b></td></tr>
</table>
</div> 
<div>
Selected Task: <label id='lbl_sel'>-</label><br />
<label style='font-weight:normal;'>Click Index to select/deselect a row.<br />
Click an element on <u>the selected row</u> to change its value.</label>
</div>
<div data-role='controlgroup' data-type='horizontal'>
<button data-theme='b' data-mini='true' id='btn_app'>Append</button>
<button data-theme='c' data-mini='true' id='btn_ins'>Insert</button>
<button data-theme='c' data-mini='true' id='btn_dele'>Delete</button>
<button data-theme='a' data-mini='true' id='btn_clr'>Clear</button>
</div>
</li>
<li>
<table>
<tr><td><b>Device Key:</b></td><td><input type='password' size=20 maxlength=32 id='dkey' data-mini='true'></td></tr>
<tr><td colspan=2><label id='msg'></label></td></tr> 
</table>
<div data-role='controlgroup' data-type='horizontal'>
<button data-theme='b' id='btn_submit'>Submit</button>
<button data-theme='c' id='btn_run'>Run Now</button>
<button data-theme='c' id='btn_delp'>Remove</button>
</div>
</li>
</ul>
</div>
</div>
<script>
function toggleDayType(){
$('#div_interval').hide();
$('#div_weekly').hide();
if(eval_cb('#weekly'))  $('#div_weekly').show();
if(eval_cb('#interval'))  $('#div_interval').show();
}
function toggleSTType() {
$('#div_repeat').hide();
$('#div_fixed').hide();      
if(eval_cb('#fixed')) $('#div_fixed').show();
if(eval_cb('#repeat')) $('#div_repeat').show();
}
function clear_msg() {$('#msg').text('');}  
function show_msg(s) {$('#msg').text(s).css('color','red'); setTimeout(clear_msg, 2000);}
function err_msg(e) {
if(e===3) return 'mismatch';
if(e===16) return 'missing data';
if(e===17) return 'out of bound';
return e;
}  
function show_result(jd,m,t) {
if(jd.result==2) show_msg('Device key is incorrect!');
else if(jd.result!=1) show_msg('Error: '+err_msg(jd.result)+', item: '+jd.item+'.');
else {
$('#msg').html('<font color=green>'+m+'</font>');
if(t>0) setTimeout(close, t);
}
}
var selected = -1;
var tasks=[];
var pid=-1;
function reselect_e() {
var tmp=selected;
select_e(-1);
select_e(tmp);
}
function select_e(id) {
if(selected==id) {
if(selected>0) {
$('#e'+selected).children('td').css('background-color', '');
selected=-1;
}
} else {
if($('#e'+id)) {
$('#e'+id).children('td').css('background-color', '#FFFF00');
}
if(selected>0) {
$('#e'+selected).children('td').css('background-color', '');
}
selected=id;
}
$('#lbl_sel').text(selected>0?selected:'-');
$('#btn_mod').button(selected>0?'enable':'disable');
$('#btn_dele').button(selected>0?'enable':'disable');
$('#btn_ins').button(selected>0?'enable':'disable');
}
function mod_e(id,n){
if(selected==id) {
e=tasks[id-1];
if(n=='s1') e.s1=1-e.s1;
if(n=='s2') e.s2=1-e.s2;
if(n=='s3') e.s3=1-e.s3;
if(n=='dur') {
var x=prompt('Enter duration (in seconds)', e.dur);
if(x) {
var d=parseInt(x);
if(d>0&&d<=64800) e.dur=d;
}
}
tasks[id-1]=e;
sync_e();
reselect_e();
}
}
function td_b(x,i,n) {return '<td onclick=mod_e('+i+',"'+n+'")>'+(x?'On':'-')+'</td>';}
function td_s(x,i,n) {return '<td onclick=mod_e('+i+',"'+n+'")>'+x+'</td>';}
function two_digits(x) {return ''+(x/10>>0)+(x%10);}
function convt(sec) {
var h=sec/3600>>0;
var m=(sec%3600)/60>>0;
var s=sec%60;
return ''+two_digits(h)+':'+two_digits(m)+':'+two_digits(s);
}
function append_e(e) {
var id=$('#tab_tasks tr').length;
var r='<tr id=e'+id+'><td onclick=select_e('+id+')>'+id+'</td>';
r+=td_b(e.s1,id,'s1');
r+=td_b(e.s2,id,'s2');
r+=td_b(e.s3,id,'s3');
r+=td_s(convt(e.dur),id,'dur');
r+='</tr>';
$('#tab_tasks').append(r);
}
function sync_e() {
$('#tab_tasks').find('tr:gt(0)').remove();
var i;
for(i=0;i<tasks.length;i++) append_e(tasks[i]);

}
function eval_cb(n) {
return $(n).is(':checked')?1:0;
}
function get_daytype() {
return eval_cb('#weekly')?0:1;
}
function set_daytype(v) {
$(v?'#interval':'#weekly').attr('checked',true);
$('#weekly').checkboxradio('refresh');
$('#interval').checkboxradio('refresh');
toggleDayType();
}
function get_days() {
var i, days=[0,0];
if(eval_cb('#weekly')) {
for(i=0;i<7;i++) {
days[0]|=(eval_cb('#wd'+i)<<i);
}
} else {
days[0]=parseInt($('#intd').val());
days[1]=parseInt($('#intn').val());
}
return days;
}
function set_days(v,days) {
if(v) {
$('#intn').val(days[1]);
$('#intd').val(days[0]);
} else {
for(i=0;i<7;i++) {
$('#wd'+i).attr('checked',(days[0]&(1<<i))?true:false).checkboxradio('refresh');
}
}  
}
function get_restr() {
var i;
for(i=0;i<3;i++)
if(eval_cb('#restr'+i)) return i;
return -1;
}
function set_restr(r) {
$('#restr'+r).attr('checked',true);
var i;
for(i=0;i<3;i++) $('#restr'+i).checkboxradio('refresh');
}
function e2i(e){
var u=(e.dur<<8)|(e.s1<<0)|(e.s2<<1)|(e.s3<<2);
return u;
}
function i2e(u){
var e=[];
e.s1=((u>>0)&0x01);e.s2=((u>>1)&0x01);e.s3=((u>>2)&0x01);
e.dur=(u>>8);
return e;
}
function get_sttype() {
if(eval_cb('#stnone')) return 0;
if(eval_cb('#fixed'))  return 1;
if(eval_cb('#repeat')) return 2;
}
function set_sttype(v) {
if(v==1) $('#fixed').attr('checked',true);
else if(v==2) $('#repeat').attr('checked',true);
else $('#stnone').attr('checked',true);
$('#stnone').checkboxradio('refresh');
$('#fixed').checkboxradio('refresh');
$('#repeat').checkboxradio('refresh');
toggleSTType();
}
function get_st(i) {
var h=parseInt($('#st'+i+'h').val());
var m=parseInt($('#st'+i+'m').val());
if((h>=0)&&(h<24)&&(m>=0)&&(m<60)) return h*60+m;
else return -1;
}
function get_sts() {
var i, sts=[get_st(0),-1,-1,-1,-1];
if(eval_cb('#fixed'))
for(i=1;i<=4;i++) sts[i]=get_st(i);
else if(eval_cb('#repeat')) { 
sts[1]=parseInt($('#repk').val());
if(!(sts[1]>=0)) sts[1]=0;
sts[2]=parseInt($('#rept').val());
if(!(sts[2]>0)) sts[2]=0;
}
return sts;
}
function set_sts(v,sts) {
var i;
if(v==2) {
var h=sts[0]/60>>0;
var m=sts[0]%60;
$('#st'+0+'h').val(two_digits(h));
$('#st'+0+'m').val(two_digits(m));
$('#repk').val(sts[1]);
$('#rept').val(sts[2]);
} else {
for(i=0;i<=4;i++) {
if(sts[i]>=0) {
var h=sts[i]/60>>0;
var m=sts[i]%60;
$('#st'+i+'h').val(two_digits(h));
$('#st'+i+'m').val(two_digits(m));
} else {
$('#st'+i+'h').val('-');
$('#st'+i+'m').val('-');
}
}
}
}
$('#btn_cancel').click(function(){
if(confirm('Quit editing this program?')) close();
});
$('#btn_submit').click(function(){
if($('#name').val().length==0) {show_msg('Program name is empty!');return;}
if(tasks.length==0) {show_msg('Program has no tasks!');return;}
if(get_st(0)==-1) {show_msg('Invalid start time!');return;}
var days=get_days();
if(eval_cb('#weekly')&&days[0]==0) {show_msg('No weekday selected!');return;}
if(eval_cb('#interval')) {
if(!(days[1]>=1)) {show_msg('Invalid interval day!');return;}
if(!(days[0]>=0&&days[0]<days[1])) {show_msg('Invalid starting in days!');return;}
}
var sts=get_sts();    
if(eval_cb('#repeat')) {
if(!(sts[2]>=1)) {show_msg('Invalid repeat interval!');return;}
if(!(sts[1]>=0)) {show_msg('Invalid repeat times!');return;}
}
if(confirm('Submit changes to this program?')) {
var comm='cp?dkey='+encodeURIComponent($('#dkey').val())+'&pid='+pid;
var config=(eval_cb('#en'))|(get_daytype()<<1)|(get_restr()<<2)|(get_sttype()<<4);
config=config|(days[0]<<8)|(days[1]<<16);
comm+='&config='+config;
get_sts();
comm+='&sts=[';
var i;
for(i=0;i<sts.length-1;i++) {
comm+=sts[i]+',';
}
comm+=sts[i]+']';
comm+='&nt='+tasks.length;
comm+='&pt=[';
for(i=0;i<tasks.length;i++){
comm+=e2i(tasks[i]);
if(i!=tasks.length-1) comm+=',';
}
comm+=']&name='+encodeURIComponent($('#name').val());
$.getJSON(comm, function(jd){show_result(jd,'Successfully submitted.',2000);});
}
});
$('#btn_run').click(function(){
if(pid==-1) {show_msg('Please submit this program first.'); return;}
if(confirm('Run this program now?')) {
var comm='rp?dkey='+encodeURIComponent($('#dkey').val())+'&pid='+pid;
$.getJSON(comm, function(jd){show_result(jd,'Program has started.',2000);});
}
});
$('#btn_delp').click(function(){
if(confirm('Remove this program?')) {
var comm='dp?dkey='+encodeURIComponent($('#dkey').val())+'&pid='+pid;
$.getJSON(comm, function(jd){show_result(jd,'Program removed.',2000);});
}
});
$('#page_prog').on('pagebeforeshow', function(){
var ss=window.location.search.split('pid=');
pid=-1;
if(ss.length==2) {
var tmp=parseInt(ss[1]);pid=(tmp>=0)?tmp:-1;
}
$('#lbl_pid').text((pid==-1)?'New Program':'Program '+(pid+1));
$('#btn_run').button((pid==-1)?'disable':'enable');
$('#btn_delp').button((pid==-1)?'disable':'enable');  
$('#btn_mod').button('disable');
$('#btn_dele').button('disable');
$('#btn_ins').button('disable');
if(pid>=0) {
$.ajax('/jp', {async:false, dataType:'json', success: function(jdata) {
var jd=jdata.progs[pid];
if(!jd) return;
$('#en').attr('checked',(jd.config&0x01)?true:false).checkboxradio('refresh');
var daytype=(jd.config>>1)&0x01;
set_daytype(daytype);
var days=[(jd.config>>8)&0xFF,(jd.config>>16)&0xFF];
set_days(daytype, days);
set_restr((jd.config>>2)&0x03);
var sttype=(jd.config>>4)&0x03;
set_sttype(sttype);
set_sts(sttype,jd.sts);

tasks.length=0;
for(i=0;i<jd.nt;i++) {
e=i2e(jd.pt[i]);
tasks.push(e);
}
$('#name').val(jd.name);
sync_e();
}});
}    
});
$('#btn_clr').click(function(){
if(confirm('Delete all program tasks?')) {
tasks.length=0;
sync_e();
select_e(-1);
}
});
$('#btn_app').click(function() {
var e=[];
e.s1=0;e.s2=0;e.s3=0;e.dur=60;
tasks.push(e);
sync_e();
select_e(tasks.length);
});
$('#btn_ins').click(function() {
var e=[];
e.s1=0;e.s2=0;e.s3=0;e.dur=60;
if(selected>0) {
tasks.splice(selected-1,0,e); 
sync_e();
select_e(selected+1);
} 
});
$('#btn_dele').click(function(){
if(selected>0) {
if(confirm('Delete the selected task?')) {
tasks.splice(selected-1,1);
sync_e();
select_e(-1);
}
}
});
$('table').on('selectstart', function (event) {
event.preventDefault();
});
</script>
</body>)";

const char settings_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi Options</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' type='text/css'>
<script src='http://code.jquery.com/jquery-1.9.1.min.js' type='text/javascript'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js' type='text/javascript'></script>
</head>
<body>
<div data-role='page' id='page_opts'>
<div data-role='header'><h3>Edit Options</h3>
<a data-role='button' id='btn_cancel' data-theme='a' data-icon='delete' data-iconpos='left' class='ui-btn-left'>Cancel</a>
</div>    
<div data-role='content'>   
<table cellpadding=2>
<tr><td><b>Time Zone:</b></td><td>
<select name='tmz' id='tmz' data-mini='true'>
<script>
function w(s) {document.write(s);}
var i;
for(i=0;i<104;i++) {
var tz=(i-48)*25;
var tzname=(tz<0)?'-':'+';
tz=(tz>0)?tz:-tz;
tzname+=''+(tz/1000>>0)+((tz/100>>0)%10)+':'+((tz/10>>0)%10)+(tz%10);
w('<option value='+i+'>GMT'+tzname+'</option>');
}
</script>
</select>
</td></tr>
<tr><td><b>Device Name:</b></td><td><input type='text' size=20 maxlength=32 id='name' data-mini='true' value='-'></td></tr>
<script>
for(i=0;i<3;i++)
w('<tr><td><b>Zone'+(i+1)+' Name:</b></td><td><input type=text size=20 maxlength=32 id=zon'+i+' data-mini=true value="Zone '+(i+1)+'"></td></tr>');
</script>
<tr><td><b>Cloud Token:</b></td><td><input type='text' size=32 maxlength=32 id='auth' data-mini='true' value='-'></td></tr>
<tr><td><b>Valve Type:</b></td><td>
<select name='sot' id='sot' data-mini='true'>
<option value=0>Latching (bi-stable)</option>
<option value=1>Non-latching (mono-stable)</option>
</select></td></tr> 
<tr><td><b>HTTP Port:</b></td><td><input type='text' size=5 maxlength=5 id='htp' value=1 data-mini='true'></td></tr>
<tr><td><b>Device Key:</b></td><td><input type='password' size=24 maxlength=32 id='dkey' data-mini='true'></td></tr>
<tr><td colspan=2><p id='msg'></p></td></tr>
</table>
<div data-role='controlgroup' data-type='horizontal'>
<a href='#' data-role='button' data-inline='true' data-theme='b' id='btn_submit'>Submit</a>      
</div>
<br />
<table>
<tr><td colspan=2><input type='checkbox' data-mini='true' id='cb_key'><label for='cb_key'>Change Device Key</label></td></tr>
<tr><td><b>New Key:</b></td><td><input type='password' size=24 maxlength=32 id='nkey' data-mini='true' disabled></td></tr>
<tr><td><b>Confirm:</b></td><td><input type='password' size=24 maxlength=32 id='ckey' data-mini='true' disabled></td></tr>      
</table>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler Bee (<a href='http://bee.opensprinkler.com' target='_blank' style='text-decoration:none'>bee.opensprinkler.com</a>)</p>
</div>   
</div>
<script>
function err_msg(e) {
if(e===3) return 'mismatch';
if(e===16) return 'missing data';
if(e===17) return 'out of bound';
return e;
}    
function clear_msg() {$('#msg').text('');}  
function show_msg(s) {$('#msg').text(s).css('color','red'); setTimeout(clear_msg, 2000);}
$('#cb_key').click(function(e){
$('#nkey').textinput($(this).is(':checked')?'enable':'disable');
$('#ckey').textinput($(this).is(':checked')?'enable':'disable');
});
$('#btn_cancel').click(function(e){ e.preventDefault(); close(); });
$('#btn_submit').click(function(e){
e.preventDefault();
if(confirm('Submit changes?')) {
var comm='co?dkey='+encodeURIComponent($('#dkey').val());
comm+='&tmz='+$('#tmz').val();
comm+='&sot='+$('#sot').val();
comm+='&htp='+$('#htp').val();
comm+='&name='+encodeURIComponent($('#name').val());
var i;
for(i=0;i<3;i++) comm+='&zon'+i+'='+encodeURIComponent($('#zon'+i).val());
comm+='&auth='+encodeURIComponent($('#auth').val());
if($('#cb_key').is(':checked')) {
if(!$('#nkey').val()) {
if(!confirm('New device key is empty. Are you sure?')) return;
}
comm+='&nkey='+encodeURIComponent($('#nkey').val());
comm+='&ckey='+encodeURIComponent($('#ckey').val());
}
$.getJSON(comm, function(jd) {
if(jd.result!=1) {
if(jd.result==2) show_msg('Check device key and try again.');
else show_msg('Error: '+err_msg(jd.result)+', item: '+jd.item);
} else {
$('#msg').html('<font color=green>Options are successfully saved. Note that<br>changes to some options may require a reboot.</font>');
setTimeout(close, 2000);
}
});
}
});
$('#page_opts').on('pagebeforeshow', function() {
$.getJSON('jo', function(jd) {
$('#tmz').val(jd.tmz).selectmenu('refresh');
$('#sot').val(jd.sot).selectmenu('refresh');
$('#htp').val(jd.htp);
$('#name').val(jd.name);
var i;
for(i=0;i<3;i++) $('#zon'+i).val(jd.zons[i]);
$('#auth').val(jd.auth);
});
});
</script>
</body>)";

const char update_html[] PROGMEM = R"(<head>
<title>OSBeeWiFi</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' type='text/css'>
<script src='http://code.jquery.com/jquery-1.9.1.min.js' type='text/javascript'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js' type='text/javascript'></script>
</head>
<body>
<div data-role='page' id='page_update'>
<div data-role='header'><h3>OSBeeWiFi Firmware Update</h3></div>
<div data-role='content'>
<form method='POST' action='/update' id='fm' enctype='multipart/form-data'>
<table cellspacing=4>
<tr><td><input type='file' name='file' accept='.bin' id='file'></td></tr>
<tr><td><b>Device key: </b><input type='password' name='dkey' size=16 maxlength=16 id='dkey'></td></tr>
<tr><td><label id='msg'></label></td></tr>
</table>
<div data-role='controlgroup' data-type='horizontal'>
<a href='#' data-role='button' data-inline='true' data-theme='a' id='btn_cancel'>Cancel</a>
<a href='#' data-role='button' data-inline='true' data-theme='b' id='btn_submit'>Submit</a>
</div>
</form>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler Bee (<a href='http://bee.opensprinkler.com' target='_blank' style='text-decoration:none'>bee.opensprinkler.com</a>)</p>
</div>  
</div>
<script>
function id(s) {return document.getElementById(s);}
function clear_msg() {id('msg').innerHTML='';}
function show_msg(s,t,c) {
id('msg').innerHTML=s.fontcolor(c);
if(t>0) setTimeout(clear_msg, t);
}  
$('#btn_cancel').click(function(e){
e.preventDefault(); close();
});
$('#btn_submit').click(function(e){
var files= id('file').files;
if(files.length==0) {show_msg('Please select a file.',2000,'red'); return;}
if(id('dkey').value=='') {
if(!confirm('You did not input a device key. Are you sure?')) return;
}
var btn = id('btn_submit');
show_msg('Uploading. Please wait...',0,'green');
var fd = new FormData();
var file = files[0];
fd.append('file', file, file.name);
fd.append('dkey', id('dkey').value);
var xhr = new XMLHttpRequest();
xhr.onreadystatechange = function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.result==1) {
show_msg('Update is successful. Rebooting. Please wait...',0,'green');
setTimeout(close, 10000);
} else if (jd.result==2) {
show_msg('Check device key and try again.', 0, 'red');
} else {
show_msg('Update failed.',0,'red');
}
}
};
xhr.open('POST', 'update', true);
xhr.send(fd);
});
</script>
</body>)";

