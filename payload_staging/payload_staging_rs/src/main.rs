use actix_web::{get, App, HttpServer, Responder, HttpResponse, web};

// Add the module for our WinInet wrapper
mod wininet_wrapper;

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
            </body>
            <hr>
        </html>
        "
    )
}

// Adding a new endpoint that uses our WinInet wrapper
#[get("/fetch/{url:.*}")]
async fn fetch_url(path: web::Path<String>) -> impl Responder {
    let url = path.into_inner();
    
    // Make sure URL is properly formatted
    let full_url = if !url.starts_with("http://") && !url.starts_with("https://") {
        format!("https://{}", url)
    } else {
        url
    };
    
    // Use our WinInet wrapper to download the URL
    match wininet_wrapper::download_url(&full_url, "Rust WinInet Client/1.0") {
        Ok(data) => {
            // Return the downloaded content
            HttpResponse::Ok()
                .content_type("application/octet-stream")
                .body(data)
        },
        Err(error) => {
            // Return the error message
            HttpResponse::InternalServerError()
                .content_type("text/plain")
                .body(format!("Failed to download URL: {}", error))
        }
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()>{
    HttpServer::new(|| {
        App::new()
            .service(hello)
            .service(fetch_url)
    })
    .bind(("127.0.0.1", 8080))?
    .run()
    .await
    
}
