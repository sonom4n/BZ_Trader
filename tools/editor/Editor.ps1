# BZ_Trader - Traveling_Trader on Wheels - el editor con un servidor local.
# Para el server donde esta esta carpeta (<server>\@BZ_Trader\tools\editor): sin elegir carpetas, el editor lee y escribe
# directo en ese server, aunque este en Program Files (si Windows le da permiso de escritura al usuario).
# Escucha SOLO en esta PC (loopback), en el primer puerto libre desde -Puerto. Cada pedido a la API lleva la clave de esta
# sesion (va en el enlace que se abre en el navegador; sin ella, 403) y no sale de la carpeta del server. Fuera de la raiz
# de un server (sin DayZServer_x64.exe ni mpmissions al lado del mod) lo avisa y no lee ni escribe nada.
# Lo abre Editor.bat. Cerrar la ventana lo termina; cerrar el editor en el navegador, tambien: cada pestana late y
# avisa al irse, y sin ninguna pestana viva termina solo (ver Cerrado).
param(
    [int]$Puerto = 8765,        # el primero que prueba; si esta ocupado, los siguientes
    [switch]$NoAbrir,           # no abre el navegador: solo escribe el enlace
    [int]$GraciaS = 10,         # se fue la ultima pestana: espera esto a que vuelva (una recarga, F5) antes de terminar
    [int]$TopeLatidoS = 180     # una pestana que no late en esto cuenta como cerrada (late cada 15 s; una oculta, hasta 1 por minuto)
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2

$EditorDir = $PSScriptRoot
$ModDir = Split-Path -Parent (Split-Path -Parent $EditorDir)
$Mod = Split-Path -Leaf $ModDir
$ServerDir = [IO.Path]::GetFullPath((Split-Path -Parent $ModDir))
if (-not $ServerDir.EndsWith("\")) { $ServerDir = $ServerDir + "\" }
# la ruta que se muestra: sin la barra final, salvo la raiz de un disco (como D:\)
$ServerVer = $ServerDir.TrimEnd("\")
if ($ServerVer -match "^[A-Za-z]:$") { $ServerVer = $ServerDir }
# Al lado del mod tiene que estar DayZServer_x64.exe o mpmissions (con eso alcanza la copia de un host). Si no (la carpeta
# del Workshop, la de mods del cliente, un @BZ_Trader dentro de otra carpeta), lo dicen esta ventana y la pagina, y la API
# no lee ni escribe nada.
$EsServer = [IO.File]::Exists((Join-Path $ServerDir "DayZServer_x64.exe")) -or [IO.Directory]::Exists((Join-Path $ServerDir "mpmissions"))
$ModVer = $ModDir.TrimEnd("\")
$Clave = [guid]::NewGuid().ToString("N") + [guid]::NewGuid().ToString("N")
$Ip = [System.Net.IPAddress]::Loopback.ToString()
$Utf8 = New-Object System.Text.UTF8Encoding($false)
$MaxCuerpo = 64MB
$Tipos = @{ ".html" = "text/html; charset=utf-8"; ".js" = "text/javascript; charset=utf-8"; ".css" = "text/css; charset=utf-8";
            ".pdf" = "application/pdf"; ".md" = "text/markdown; charset=utf-8"; ".png" = "image/png"; ".svg" = "image/svg+xml"; ".ico" = "image/x-icon" }
# las pestanas del editor abiertas (id -> su ultimo latido, UTC); desde cuando no queda ninguna; si alguna latio
$Pestanas = @{}
$SinPestanasDesde = $null
$AlgunaVez = $false

# Se cerro el editor en el navegador? Recien despues de que alguna pestana latio (antes, el navegador todavia abre).
# Una pestana que no late en $TopeLatidoS cuenta como cerrada (el navegador se cerro de golpe o el aviso no llego). Sin
# ninguna, espera $GraciaS desde que se fue la ultima: una recarga (F5) avisa al irse y vuelve a latir enseguida.
function Cerrado() {
    if (-not $script:AlgunaVez) { return $false }
    $ahora = [DateTime]::UtcNow
    foreach ($t in @($script:Pestanas.Keys)) {
        if (($ahora - $script:Pestanas[$t]).TotalSeconds -gt $TopeLatidoS) {
            $script:Pestanas.Remove($t)
            if ($script:Pestanas.Count -eq 0) { $script:SinPestanasDesde = $ahora }
        }
    }
    if ($script:Pestanas.Count -gt 0 -or $null -eq $script:SinPestanasDesde) { return $false }
    return (($ahora - $script:SinPestanasDesde).TotalSeconds -ge $GraciaS)
}

# ------------------------------------------------------------------ la ruta pedida, adentro del server (o $null)
function Resolver([string]$rel) {
    if ($null -eq $rel) { $rel = "" }
    $r = $rel.Replace("/", "\")
    if ($r.StartsWith("\") -or $r.Contains(":") -or $r.Contains([string][char]0)) { return $null }
    foreach ($p in $r.Split("\")) { if ($p -eq "..") { return $null } }
    $full = [IO.Path]::GetFullPath([IO.Path]::Combine($ServerDir, $r))
    if ($full.TrimEnd("\") -ieq $ServerDir.TrimEnd("\")) { return $ServerDir }
    if (-not $full.StartsWith($ServerDir, [StringComparison]::OrdinalIgnoreCase)) { return $null }
    return $full
}

# los parametros de la URL, decodificados como UTF-8 (encodeURIComponent del editor)
function Param($req, [string]$nombre) {
    $q = $req.Url.Query
    if (-not $q) { return $null }
    foreach ($par in $q.TrimStart("?").Split("&")) {
        $i = $par.IndexOf("=")
        if ($i -lt 0) { continue }
        if ([Uri]::UnescapeDataString($par.Substring(0, $i)) -eq $nombre) { return [Uri]::UnescapeDataString($par.Substring($i + 1)) }
    }
    return $null
}

function Responder($ctx, [int]$codigo, [byte[]]$bytes, [string]$tipo, $extra) {
    $res = $ctx.Response
    $res.StatusCode = $codigo
    $res.ContentType = $tipo
    $res.Headers["Cache-Control"] = "no-store"
    $res.Headers["X-Content-Type-Options"] = "nosniff"
    if ($extra) { foreach ($k in $extra.Keys) { $res.Headers[$k] = [string]$extra[$k] } }
    $res.ContentLength64 = $bytes.Length
    if ($ctx.Request.HttpMethod -ne "HEAD" -and $bytes.Length -gt 0) { $res.OutputStream.Write($bytes, 0, $bytes.Length) }
}
function Json($ctx, [int]$codigo, $obj) {
    Responder $ctx $codigo ($Utf8.GetBytes((ConvertTo-Json -InputObject $obj -Compress -Depth 4))) "application/json; charset=utf-8" $null
}
function Mtime($info) { return [DateTimeOffset]::new($info.LastWriteTimeUtc).ToUnixTimeMilliseconds() }

# ------------------------------------------------------------------ la API: info, listar, leer, escribir
function Api($ctx, [string]$que) {
    $req = $ctx.Request
    if ($que -eq "info") {
        Json $ctx 200 @{ servidor = $ServerVer; mod = $Mod; version = 1; esServer = $EsServer; modDir = $ModVer }
        return
    }
    # el latido de cada pestana y su aviso al irse (tambien fuera de la raiz de un server: la ventana termina igual)
    if ($que -eq "latido" -or $que -eq "adios") {
        $tab = [string](Param $req "tab")
        if ($tab -notmatch "^[a-z0-9]{4,40}$") { Json $ctx 400 @{ error = "tab" }; return }
        if ($que -eq "latido") {
            $script:Pestanas[$tab] = [DateTime]::UtcNow
            $script:AlgunaVez = $true
            $script:SinPestanasDesde = $null
        } else {
            [void]$script:Pestanas.Remove($tab)
            if ($script:Pestanas.Count -eq 0) { $script:SinPestanasDesde = [DateTime]::UtcNow }
        }
        Json $ctx 200 @{ ok = $true; pestanas = $script:Pestanas.Count }
        return
    }
    if (-not $EsServer) { Json $ctx 409 @{ error = "no_es_server"; detalle = $ModVer }; return }
    $rel = Param $req "ruta"
    $full = Resolver $rel
    if (-not $full) { Json $ctx 400 @{ error = "ruta"; detalle = "fuera del server: " + $rel }; return }
    if ($que -eq "listar") {
        if (-not [IO.Directory]::Exists($full)) { Json $ctx 404 @{ error = "no_existe" }; return }
        $out = New-Object System.Collections.ArrayList
        foreach ($e in (New-Object IO.DirectoryInfo($full)).EnumerateFileSystemInfos()) {
            $dir = ($e.Attributes -band [IO.FileAttributes]::Directory) -ne 0
            $tam = 0
            if (-not $dir) { $tam = $e.Length }
            [void]$out.Add(@{ n = $e.Name; d = $dir; t = $tam; m = (Mtime $e) })
        }
        Json $ctx 200 ([object[]]$out.ToArray())
        return
    }
    if ($que -eq "leer") {
        if (-not [IO.File]::Exists($full)) { Json $ctx 404 @{ error = "no_existe" }; return }
        $max = 0
        [void][long]::TryParse([string](Param $req "max"), [ref]$max)
        # FileShare ReadWrite: el RPT lo esta escribiendo el server
        $fs = [IO.File]::Open($full, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        try {
            $largo = $fs.Length
            if ($max -gt 0 -and $max -lt $largo) { $largo = $max }
            $buf = New-Object byte[] $largo
            $leidos = 0
            while ($leidos -lt $largo) {
                $n = $fs.Read($buf, $leidos, $largo - $leidos)
                if ($n -le 0) { break }
                $leidos += $n
            }
        } finally { $fs.Dispose() }
        Responder $ctx 200 $buf "application/octet-stream" @{ "X-Mtime" = (Mtime (New-Object IO.FileInfo($full))) }
        return
    }
    if ($que -eq "escribir") {
        if ($req.HttpMethod -ne "POST") { Json $ctx 405 @{ error = "metodo" }; return }
        # solo lo que escribe el editor: .json y sus respaldos (.json.pre_editor_<fecha>)
        $nombre = [IO.Path]::GetFileName($full)
        if ($nombre -notmatch "\.json(\.pre_editor_\d{6}_\d{6})?$") { Json $ctx 400 @{ error = "tipo"; detalle = "solo .json" }; return }
        if ($req.ContentLength64 -gt $MaxCuerpo) { Json $ctx 413 @{ error = "grande" }; return }
        # PowerShell 5.1 (.NET Framework) no escribe rutas de 260 caracteres o mas: que lo diga, en vez de "no se encuentra"
        if ($full.Length -ge 260) { Json $ctx 400 @{ error = "larga"; detalle = "path longer than Windows allows here (" + $full.Length + " characters): " + $full }; return }
        $ms = New-Object IO.MemoryStream
        $req.InputStream.CopyTo($ms)
        try {
            $dir = [IO.Path]::GetDirectoryName($full)
            if (-not [IO.Directory]::Exists($dir)) { [void][IO.Directory]::CreateDirectory($dir) }
            [IO.File]::WriteAllBytes($full, $ms.ToArray())
        } catch [UnauthorizedAccessException] {
            Json $ctx 403 @{ error = "sin_permiso"; detalle = $_.Exception.Message }
            return
        } catch {
            Json $ctx 500 @{ error = "io"; detalle = $_.Exception.Message }
            return
        }
        Write-Host ("  escrito / written: " + $full.Substring($ServerDir.Length))
        Json $ctx 200 @{ ok = $true; bytes = $ms.Length }
        return
    }
    Json $ctx 404 @{ error = "api" }
}

# ------------------------------------------------------------------ cada pedido
function Atender($ctx) {
    $req = $ctx.Request
    # solo con el Host de esta PC y este puerto (una pagina de afuera no puede hablarle con otro nombre)
    $hostPedido = ([string]$req.UserHostName).ToLowerInvariant()
    if ($HostsOk -notcontains $hostPedido) { Json $ctx 403 @{ error = "host" }; return }
    $camino = $req.Url.AbsolutePath
    if ($camino.StartsWith("/api/")) {
        $claveOk = ($req.Headers["X-BZT-Key"] -eq $Clave)
        # el aviso al irse va por sendBeacon, que no lleva encabezados propios: solo ese trae la clave en la URL
        if (-not $claveOk -and $camino -eq "/api/adios") { $claveOk = ((Param $req "k") -eq $Clave) }
        if (-not $claveOk) { Json $ctx 403 @{ error = "clave" }; return }
        $origen = [string]$req.Headers["Origin"]
        if ($origen -and ($OrigenesOk -notcontains $origen.ToLowerInvariant())) { Json $ctx 403 @{ error = "origen" }; return }
        Api $ctx $camino.Substring(5)
        return
    }
    if ($req.HttpMethod -ne "GET" -and $req.HttpMethod -ne "HEAD") { Json $ctx 405 @{ error = "metodo" }; return }
    if ($camino -eq "/") { $ctx.Response.Redirect("/editor_bz_trader.html"); return }
    # los archivos del editor; el manual, de la raiz del mod (el editor lo pide como ../../MANUAL_BZ_Trader.pdf)
    $nombre = $camino.TrimStart("/")
    $ext = [IO.Path]::GetExtension($nombre).ToLowerInvariant()
    if ($nombre -notmatch "^[A-Za-z0-9_.\-]+$" -or $nombre.StartsWith(".") -or -not $Tipos.ContainsKey($ext)) { Json $ctx 404 @{ error = "no_existe" }; return }
    $f = Join-Path $EditorDir $nombre
    if (-not [IO.File]::Exists($f) -and $nombre -like "MANUAL_*") { $f = Join-Path $ModDir $nombre }
    if (-not [IO.File]::Exists($f)) { Json $ctx 404 @{ error = "no_existe" }; return }
    Responder $ctx 200 ([IO.File]::ReadAllBytes($f)) $Tipos[$ext] $null
}

# ------------------------------------------------------------------ arranque: el primer puerto libre
$Escucha = $null
for ($p = $Puerto; $p -lt $Puerto + 50; $p++) {
    $c = New-Object System.Net.HttpListener
    $c.Prefixes.Add("http://" + $Ip + ":" + $p + "/")
    $c.Prefixes.Add("http://localhost:" + $p + "/")
    try { $c.Start(); $Escucha = $c; break } catch { $c.Close() }
}
if (-not $Escucha) {
    Write-Host ("No free port from " + $Puerto + " to " + ($Puerto + 49) + " / Ningun puerto libre entre " + $Puerto + " y " + ($Puerto + 49))
    exit 1
}
$HostsOk = @(($Ip + ":" + $p), ("localhost:" + $p))
$OrigenesOk = @(("http://" + $Ip + ":" + $p), ("http://localhost:" + $p))
$Url = "http://" + $Ip + ":" + $p + "/editor_bz_trader.html#k=" + $Clave

Write-Host ""
if ($EsServer) {
    Write-Host ("  BZ_Trader editor for the server " + $ServerVer + " - close this window to stop")
    Write-Host ("  Editor de BZ_Trader para el server " + $ServerVer + " - cerra esta ventana para terminar")
} else {
    # en la carpeta del Workshop el mod se llama por su numero, sin @
    $NombreEn = ""; $NombreEs = ""
    if (-not $Mod.StartsWith("@")) { $NombreEn = " (here the folder is called " + $Mod + ")"; $NombreEs = " (aca la carpeta se llama " + $Mod + ")" }
    Write-Host ("  NOT IN THE ROOT OF A SERVER: " + $ModVer + " has no DayZServer_x64.exe or mpmissions next to it. Copy @BZ_Trader" + $NombreEn + " to the server folder, next to DayZServer_x64.exe, and open the editor from there.") -ForegroundColor Yellow
    Write-Host ("  NO ESTA EN LA RAIZ DE UN SERVER: " + $ModVer + " no tiene DayZServer_x64.exe ni mpmissions al lado. Copia @BZ_Trader" + $NombreEs + " a la carpeta del server, al lado de DayZServer_x64.exe, y abri el editor desde ahi.") -ForegroundColor Yellow
    Write-Host "  Close this window to stop / cerra esta ventana para terminar"
}
Write-Host ""
Write-Host ("  " + $Url)
Write-Host ""
if (-not $NoAbrir) { Start-Process $Url }

$Fin = $false
try {
    while ($Escucha.IsListening) {
        $ar = $Escucha.BeginGetContext($null, $null)
        while (-not $ar.AsyncWaitHandle.WaitOne(500)) {
            if (Cerrado) { $Fin = $true; break }
        }
        if ($Fin) {
            Write-Host ""
            Write-Host "  editor cerrado: termina / editor closed: exiting"
            Start-Sleep -Seconds 2
            break
        }
        $ctx = $Escucha.EndGetContext($ar)
        try { Atender $ctx }
        catch { try { Json $ctx 500 @{ error = "interno"; detalle = $_.Exception.Message } } catch { } }
        finally { try { $ctx.Response.Close() } catch { } }
    }
} finally {
    $Escucha.Stop()
    $Escucha.Close()
}
# sin error: Editor.bat no se queda en "presione una tecla" y la ventana se cierra
exit 0
