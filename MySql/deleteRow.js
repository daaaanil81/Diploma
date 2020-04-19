var mysql = require('mysql');
const date = new Date();


console.log(date.getFullYear());
var con = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '',
    database: 'LiveStream'
});

con.connect(function(err) {
    if (err) throw err;
    console.log("Connected!");
    var ip = "10.168.0.50";
    var sql = "DELETE FROM camers WHERE ip='" + ip + "'";
    con.query(sql, function (err, result) {
        if (err) throw err;
        console.log("Number of records deleted: " + result.affectedRows);
    });
    con.end(function(err) {
        if (err) {
            console.log(err.message);
        }
    });
});

