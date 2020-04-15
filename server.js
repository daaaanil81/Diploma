const fs = require('fs');
const https = require('https');
const express = require('express');
const path = require('path');
const host = process.argv.slice(2)[0];
var bodyParser = require("body-parser");

const app = express();
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());
// Certificate
const privateKey = fs.readFileSync('./certificate/danil_petrov.key');
const certificate = fs.readFileSync('./certificate/danil_petrov.cert');

const credentials = {
	key: privateKey,
	cert: certificate,
};
app.use(express.static('public'));

app.post('/', (req, res) => {
    var user_name = req.body.login;
    var password = req.body.pass;
    console.log("User name = "+user_name+", password is "+password);
    if(user_name == 'root' && password == 'pass')
    {
        res.sendFile('admin.html', {root: path.join(__dirname, 'public')});
    }
    else
    {
	    res.sendFile( __dirname + '/public/index.html');
    }
});
app.get('/about', (req, res) =>{
    res.sendFile( __dirname + '/public/about.html');
});

// Starting both http & https servers
const httpsServer = https.createServer(credentials, app);


httpsServer.listen(3000, host, () => {
	console.log('HTTPS Server running on port '+ host +':3000');
});
