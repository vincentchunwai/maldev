use actix_web::{get, App, HttpServer, Responder, HttpResponse, web};
use actix_files as fs;

#[get("/")]
async fn hello() -> impl Responder {
    HttpResponse::Ok().body(
        "
        <html>
            <head>
                <title> Directory Listing for / </title>
            </head>
            <body>
                <h1> Directory Listing for / </h1>
                <a href=\"/static/calc.bin\" download=\"calc.bin\"> Download calc.bin </a>
            </body>
            <hr>
        </html>
        "
    )
}


#[actix_web::main]
async fn main() -> std::io::Result<()>{
    
    HttpServer::new(|| {
        App::new()
            .service(fs::Files::new("/static", "./static").show_files_listing())
            .service(hello)
    })
    .bind(("127.0.0.1", 8080))?
    .run()
    .await
    
}
