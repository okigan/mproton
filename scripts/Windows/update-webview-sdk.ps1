Invoke-WebRequest https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.864.35 -OutFile Microsoft.Web.WebView2.Sdk.zip

Expand-Archive .\Microsoft.Web.WebView2.Sdk.zip

Invoke-WebRequest https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.864.35 -OutFile Microsoft.Web.WebView2.Sdk.zip
Expand-Archive ./Microsoft.Web.WebView2.Sdk.zip
Copy-Item ./Microsoft.Web.WebView2.Sdk/build/native/x64/WebView2Loader.dll -Destination ./cmd/exampleapp
