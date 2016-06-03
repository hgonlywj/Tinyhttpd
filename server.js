var http = require("http"); 
 
    var i = 0; 
    http.createServer(function(request, response) {
    console.log("\n-------------" + (i++) +"----------\n");
    console.log(request);
    console.log("\n---------------" + i +"--------\n");
    response.writeHead(200, {"Content-Type": "text/plain"}); 
 
    response.write("Hello World"); 
 
    response.end(); 
 
}).listen(4000);