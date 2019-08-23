const n_static = require('node-static');
const fs = require('fs');
const file = new n_static.Server('./index.html');
const host = process.argv.slice(2)[0];
const port = 8888;
const app = require('https').createServer({
    key: fs.readFileSync('./certificate/test_cer.key'),
    cert: fs.readFileSync('./certificate/test_cer.crt')
},handler);
app.listen(port, host,function () {
    console.log('Server listening to ',host, ':', port);
});
function handler (req, res) {
    file.serve(req, res);
}
process.stdin.resume();
process.on('SIGINT', function() {
    process.exit();
});