param(
    [int]$Port = 52888,
    [switch]$AddFirewallRule
)

$ErrorActionPreference = 'Stop'

Write-Host "== Local IPv4 addresses =="
Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -ne '127.0.0.1' -and $_.PrefixOrigin -ne 'WellKnown' } |
    Select-Object InterfaceAlias, IPAddress, PrefixLength |
    Format-Table -AutoSize

Write-Host "== TCP listener on port $Port =="
$tcp = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($tcp) {
    $tcp | Select-Object LocalAddress, LocalPort, State, OwningProcess | Format-Table -AutoSize
} else {
    Write-Warning "No TCP listener found on port $Port. Start CrossDesk and switch to IP direct mode."
}

Write-Host "== UDP endpoint on port $Port =="
$udp = Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue
if ($udp) {
    $udp | Select-Object LocalAddress, LocalPort, OwningProcess | Format-Table -AutoSize
} else {
    Write-Warning "No UDP endpoint found on port $Port. CrossDesk may not be listening yet."
}

Write-Host "== Existing CrossDesk firewall rules =="
Get-NetFirewallRule -ErrorAction SilentlyContinue |
    Where-Object { $_.DisplayName -like '*CrossDesk*' -or $_.DisplayName -like '*crossdesk*' } |
    Select-Object DisplayName, Direction, Enabled, Profile |
    Format-Table -AutoSize

if ($AddFirewallRule) {
    $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Run this script as Administrator when using -AddFirewallRule."
    }

    foreach ($rule in @(
        @{ Name = "CrossDesk LAN Test TCP $Port"; Protocol = 'TCP' },
        @{ Name = "CrossDesk LAN Test UDP $Port"; Protocol = 'UDP' }
    )) {
        if (-not (Get-NetFirewallRule -DisplayName $rule.Name -ErrorAction SilentlyContinue)) {
            New-NetFirewallRule -DisplayName $rule.Name -Direction Inbound -Action Allow -Protocol $rule.Protocol -LocalPort $Port | Out-Null
            Write-Host "Added firewall rule: $($rule.Name)"
        } else {
            Write-Host "Firewall rule already exists: $($rule.Name)"
        }
    }
}

Write-Host "Done. On the iOS controller, connect to one of the IPv4 addresses above with :$Port and the 6-character IP password."
