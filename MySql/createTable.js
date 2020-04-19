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
    var sql = "CREATE TABLE if not exists camers(id int primary key auto_increment, ip varchar(255) not null)";
    con.query(sql, function (err, result) {
        if (err) throw err;
        console.log("Table created");
    });
    con.end(function(err) {
        if (err) {
            console.log(err.message);
        }
    });
});

