const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" class="js-focus-visible">
<title>Web Page Update Demo</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  .bodytext 
  {
    font-family: "Verdana", "Arial", sans-serif;
    font-size: 24px;
    text-align: left;
    font-weight: light;
    color: #E4E6EB;
    border-radius: 5px;
    display:inline;
  }
  .bodytextButton
  {
    font-family: "Verdana", "Arial", sans-serif;
    font-size: 24px;
    text-align: left;
    font-weight: light;
    color: #E4E6EB;
    border-radius: 5px;
    display:flex;
    justify-content: center;
    align-items: center;
    padding-top: 10px;
    padding-bottom: 2px;
  }
  .navbar 
  {
    width: 100%;
    height: 50px;
    margin: 0;
    padding: 10px 0px;
    background-color: #18191A;
    color: #E4E6EB;
    border-bottom: 4px solid #04AA6D;
  }
  .fixed-top 
  {
    position: fixed;
    top: 0;
    right: 0;
    left: 0;
    z-index: 1030;
  }
  .navtitle 
  {
    float: left;
    height: 50px;
    font-family: "Verdana", "Arial", sans-serif;
    font-size: 50px;
    font-weight: bold;
    line-height: 50px;
    padding-left: 20px;
  }
  .category 
  {
    font-family: "Verdana", "Arial", sans-serif;
    font-weight: bold;
    font-size: 32px;
    line-height: 50px;
    padding: 20px 10px 0px 10px;
    color: #E4E6EB;
  }
  .heading 
  {
    font-family: "Verdana", "Arial", sans-serif;
    font-weight: normal;
    font-size: 28px;
    text-align: center;
  }
  .btn 
  {
    background-color: #04AA6D;
    border: none;
    color: white;
    padding: 10px 20px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    font-weight: bold;
    margin: 4px 2px;
    cursor: pointer;
    width: 180px;
  }
  .btn:hover
  {
  	background-color: #059862;
  	color: white;
  }
  .foot 
  {
    font-family: "Verdana", "Arial", sans-serif;
    font-size: 20px;
    position: relative;
    height:   30px;
    text-align: center;   
    color: #AAAAAA;
    line-height: 20px;
  }
  .container 
  {
    max-width: 1800px;
    margin: 0 auto;
  }

  div {text-align: center;}
  </style>
  <body style="background-color: #18191A" onload="process()">
    <header>
      <div class="navbar fixed-top">
          <div class="container">
            <div class="navtitle">My Availibility</div>
          </div>
      </div>
    </header>
    <center>
    <main class="container" style="margin-top:70px">
      <div class="category">Update your status</div>
      <br>
      <div class="bodytextButton">I am</div>
      <button type="button" class = "btn" id = "btn0" onclick="ButtonPress0()">Toggle</button>
      </div>
      <br>
      <br>
      <br>
      <div class="bodytextButton">Someone wants to meet me</div>
      <button type="button" class = "btn" id = "btn1" onclick="ButtonPress1()">Toggle</button>
      </div>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      <br>
      </div>
  
        <! removing this line break the webpage for some reason>
        <td><div class="tabledata" id = "switch"></div></td>
      
      <br>
    </main>
    
    <nav>
	    <a href = "/update">Update</a>
	  </nav>
  </center>
  </body>

  <script type = "text/javascript">
    var xmlHttp=createXmlHttpObject();
    function createXmlHttpObject()
    {
      if(window.XMLHttpRequest)
      {
        xmlHttp = new XMLHttpRequest();
      }
      else
      {
        xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
      }
      return xmlHttp;
    }

    function ButtonPress0() 
    {
      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", "BUTTON_0", false);
      xhttp.send();
    }

    function ButtonPress1() 
    {
      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", "BUTTON_1", false);
      xhttp.send(); 
    }

    function response()
    {
      var xmldoc;
      var message;
      var xmlResponse;
     
      xmlResponse = xmlHttp.responseXML;
  
      xmldoc = xmlResponse.getElementsByTagName("SW1");
      message = xmldoc[0].firstChild.nodeValue;
  
      if (message == 0)
      {
        document.getElementById("btn0").innerHTML = "Available";
        document.getElementById("btn0").style.backgroundColor = "#04AA6D";

      }
      else 
      {
        document.getElementById("btn0").innerHTML = "Busy";
        document.getElementById("btn0").style.backgroundColor = "#FF3A00";
      }
         
      xmldoc = xmlResponse.getElementsByTagName("SW2");
      message = xmldoc[0].firstChild.nodeValue;
      document.getElementById("switch").style.backgroundColor = "rgb(73, 93, 73)";
      
      if(message == 0)
      {
        document.getElementById("btn1").innerHTML = "No";   
        document.getElementById("btn1").style.backgroundColor = "#04AA6D";
      }

      else if(message == 1)
      {
        document.getElementById("btn1").innerHTML = "Yes";
        document.getElementById("btn1").style.backgroundColor = "#2563EB";
      }
      else 
      {
        document.getElementById("btn1").innerHTML = "Come";
        document.getElementById("btn1").style.backgroundColor = "#D01EDE";
      }
    }

    function process()
    { 
     if(xmlHttp.readyState == 0 || xmlHttp.readyState == 4) 
     {
        xmlHttp.open("PUT","xml",true);
        xmlHttp.onreadystatechange = response;
        xmlHttp.send(null);
      }       
      setTimeout("process()", 100);
    }
  </script>
</html>
)=====";

const char NOT_FOUND_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width=device-width, initial-scale = 1">
<style>
body 
{
  background-color: rgba(24, 25, 26, 1);
}
.button 
{
  border: none;
  color: white;
  padding: 16px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  transition-duration: 0.4s;
  cursor: pointer;
}
.button1 
{
  background-color: #04AA6D; 
  color: white; 
  border: 0px solid #04AA6D;
}
.button1:hover 
{
  background-color: #059862;
  color: white;
}
</style>
</head>
<center>
<h1 style = "color: LightGray; font-size: 60px; font-family: Century Gothic;">This page is not available</h1>
<h3 style = "color: #4CAF50; font-size: 30px; font-family: Century Gothic;">Error 404 - Not Found</span></h3>

<button class = "button button1" onclick = "gotoHomePage()">Home Page</button>

<p style ="color:orange"; id = "demo"></p>
<div>
  <h2 style = "color: #4CAF50; font-size: 20px; font-family: Century Gothic;"><span id = "val"></span>
</h2>
</div>
</center>

<script>
var delay = 5;

setInterval(function()
{
  gotoRootPage();
  }, 
  1000);
  
function gotoRootPage()
{
  delay = delay - 1;
  
  let text1 = "You will be redirected to Home Page in ";
  let text2 = delay;
  let text3 = " seconds";
  let result = text1.concat(text2, text3);
  document.getElementById("demo").innerHTML = result;
  if(delay == 0)
  {
    gotoHomePage();
  }
}

function gotoHomePage() 
{
    document.getElementById("demo").innerHTML = "Going to home page";
    window.location = 'http://'+location.hostname+'/';
}

</script>
</body>
</html>
)=====";