const fs = require('fs');
const url = require('url');
const path = require('path');
const host = process.argv.slice(2)[0];
const port = 8888;
const mimeType = {
    '.ico': 'image/x-icon',
    '.html': 'text/html',
    '.js': 'text/javascript',
    '.json': 'application/json',
    '.css': 'text/css',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.wav': 'audio/wav',
    '.mp3': 'audio/mpeg',
    '.svg': 'image/svg+xml',
    '.pdf': 'application/pdf',
    '.doc': 'application/msword',
    '.eot': 'appliaction/vnd.ms-fontobject',
    '.ttf': 'aplication/font-sfnt'
};
var options = {
    key: fs.readFileSync('certificate/test_cer.key'),
    cert: fs.readFileSync('certificate/test_cer.crt')
};
const app = require('https').createServer(options, handler);
app.listen(port, host, function () {
    console.log('Server listening to ', host, ':', port);
});

function handler(req, res) {
    //console.log(`${req.method} ${req.url}`);
    const parsedUrl = url.parse(req.url);
    //console.log(parsedUrl);
    const sanitizePath = path.normalize(parsedUrl.pathname).replace(/^(\.\.[\/\\])+/, '');
    //console.log(__dirname);
    var pathname = __dirname.substr(0, __dirname.length - 3);
    pathname = path.join(pathname, sanitizePath);
    //console.log(pathname);
    fs.exists(pathname, function (exist) {
        if (!exist) {
            res.statusCode = 404;
            res.end(`File ${pathname} not found!`);
            return;
        }
        if (fs.statSync(pathname).isDirectory()) {
            pathname += '/Sites/index.html';
        }
        fs.readFile(pathname, function (err, data) {
            const ext = path.parse(pathname).ext;
            res.setHeader('Content-type', mimeType[ext] || 'text/plain');
            res.end(data);
        });
    });
}

process.stdin.resume();
process.on('SIGINT', function () {
    process.exit();
});