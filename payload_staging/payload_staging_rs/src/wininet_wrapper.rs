use windows::core::{PCWSTR, HSTRING};
use windows::Win32::Foundation::{HANDLE, WIN32_ERROR, BOOL, GetLastError};
use windows::Win32::Networking::WinInet::{
    InternetOpenW, InternetConnectW, HttpOpenRequestW, HttpSendRequestW, 
    InternetReadFile, InternetCloseHandle, INTERNET_FLAG_SECURE, INTERNET_FLAG_RELOAD,
    INTERNET_SERVICE_HTTP, INTERNET_OPEN_TYPE_PRECONFIG, INTERNET_DEFAULT_HTTPS_PORT
};

pub struct WinInetClient {
    internet_handle: HANDLE,
}

impl WinInetClient {
    pub fn new(agent: &str) -> Result<Self, WIN32_ERROR> {
        // Convert the Rust string to a wide string for Windows API
        let agent_wide = HSTRING::from(agent);
        
        // Call InternetOpenW to initialize WinINet
        unsafe {
            let handle = InternetOpenW(
                PCWSTR::from_raw(agent_wide.as_ptr()),
                INTERNET_OPEN_TYPE_PRECONFIG,
                None,  // Default proxy
                None,  // Default proxy bypass
                0,     // No flags
            );
            
            if handle.is_invalid() {
                return Err(GetLastError());
            }
            
            Ok(Self {
                internet_handle: handle
            })
        }
    }

    pub fn connect(&self, server: &str, port: u16, secure: bool) -> Result<ConnectionHandle, WIN32_ERROR> {
        let server_wide = HSTRING::from(server);
        
        unsafe {
            let connection = InternetConnectW(
                self.internet_handle,
                PCWSTR::from_raw(server_wide.as_ptr()),
                port,
                None,  // No username
                None,  // No password
                INTERNET_SERVICE_HTTP,
                0,     // No flags
                None,  // No context
            );
            
            if connection.is_invalid() {
                return Err(GetLastError());
            }
            
            Ok(ConnectionHandle {
                connection_handle: connection,
                secure
            })
        }
    }
}

impl Drop for WinInetClient {
    fn drop(&mut self) {
        if !self.internet_handle.is_invalid() {
            unsafe {
                let _ = InternetCloseHandle(self.internet_handle);
            }
        }
    }
}

pub struct ConnectionHandle {
    connection_handle: HANDLE,
    secure: bool,
}

impl ConnectionHandle {
    pub fn open_request(&self, method: &str, url: &str) -> Result<RequestHandle, WIN32_ERROR> {
        let method_wide = HSTRING::from(method);
        let url_wide = HSTRING::from(url);
        
        // Use HTTPS flags if secure connection is requested
        let flags = if self.secure { INTERNET_FLAG_SECURE } else { 0 } | INTERNET_FLAG_RELOAD;
        
        unsafe {
            let request = HttpOpenRequestW(
                self.connection_handle,
                PCWSTR::from_raw(method_wide.as_ptr()),
                PCWSTR::from_raw(url_wide.as_ptr()),
                None,  // Use default HTTP version
                None,  // No referer
                None,  // No accept types
                flags,
                None,  // No context
            );
            
            if request.is_invalid() {
                return Err(GetLastError());
            }
            
            Ok(RequestHandle {
                request_handle: request
            })
        }
    }
}

impl Drop for ConnectionHandle {
    fn drop(&mut self) {
        if !self.connection_handle.is_invalid() {
            unsafe {
                let _ = InternetCloseHandle(self.connection_handle);
            }
        }
    }
}

pub struct RequestHandle {
    request_handle: HANDLE,
}

impl RequestHandle {
    pub fn send(&self) -> Result<(), WIN32_ERROR> {
        unsafe {
            let result = HttpSendRequestW(
                self.request_handle,
                None,  // No additional headers
                0,     // Header length
                None,  // No post data
                0,     // Post data length
            );
            
            if !result.as_bool() {
                return Err(GetLastError());
            }
            
            Ok(())
        }
    }

    pub fn read(&self, buffer: &mut [u8]) -> Result<usize, WIN32_ERROR> {
        let mut bytes_read: u32 = 0;
        
        unsafe {
            let result = InternetReadFile(
                self.request_handle,
                buffer.as_mut_ptr() as *mut core::ffi::c_void,
                buffer.len() as u32,
                &mut bytes_read,
            );
            
            if !result.as_bool() {
                return Err(GetLastError());
            }
            
            Ok(bytes_read as usize)
        }
    }
}

impl Drop for RequestHandle {
    fn drop(&mut self) {
        if !self.request_handle.is_invalid() {
            unsafe {
                let _ = InternetCloseHandle(self.request_handle);
            }
        }
    }
}

// Example usage function
pub fn download_url(url: &str, agent: &str) -> Result<Vec<u8>, String> {
    // Parse URL (simplified)
    let url_parts: Vec<&str> = url.split("://").collect();
    if url_parts.len() != 2 {
        return Err("Invalid URL format".to_string());
    }
    
    let protocol = url_parts[0];
    let secure = protocol == "https";
    
    let remaining_parts: Vec<&str> = url_parts[1].split('/').collect();
    let host = remaining_parts[0];
    
    let path = if remaining_parts.len() > 1 {
        "/".to_string() + &remaining_parts[1..].join("/")
    } else {
        "/".to_string()
    };

    // Create WinInet client
    let client = WinInetClient::new(agent)
        .map_err(|e| format!("Failed to create WinInet client: {}", e.0))?;
    
    // Connect to server
    let port = if secure { INTERNET_DEFAULT_HTTPS_PORT } else { 80 };
    let connection = client.connect(host, port, secure)
        .map_err(|e| format!("Failed to connect: {}", e.0))?;
    
    // Open request
    let request = connection.open_request("GET", &path)
        .map_err(|e| format!("Failed to open request: {}", e.0))?;
    
    // Send request
    request.send()
        .map_err(|e| format!("Failed to send request: {}", e.0))?;
    
    // Read response
    let mut response = Vec::new();
    let mut buffer = [0u8; 8192];
    
    loop {
        let bytes_read = request.read(&mut buffer)
            .map_err(|e| format!("Failed to read response: {}", e.0))?;
        
        if bytes_read == 0 {
            break;
        }
        
        response.extend_from_slice(&buffer[..bytes_read]);
    }
    
    Ok(response)
}