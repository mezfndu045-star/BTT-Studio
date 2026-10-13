# Script d'automatisation Git & GitHub pour BTT-Studio 1.0

Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  Publication BTT Studio Practice Mod 1.0 sur GitHub   " -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# Vérifier si l'identité Git est configurée
$userName = git config --get user.name
$userEmail = git config --get user.email

if (-not $userName) {
    Write-Host "[!] Aucun nom d'utilisateur Git configuré." -ForegroundColor Yellow
    $inputName = Read-Host "Entrez votre nom / pseudo GitHub"
    if ($inputName) {
        git config user.name "$inputName"
        Write-Host "[✓] user.name configuré sur '$inputName'" -ForegroundColor Green
    }
}

if (-not $userEmail) {
    Write-Host "[!] Aucun email Git configuré." -ForegroundColor Yellow
    $inputEmail = Read-Host "Entrez votre adresse email GitHub"
    if ($inputEmail) {
        git config user.email "$inputEmail"
        Write-Host "[✓] user.email configuré sur '$inputEmail'" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "Vérification du remote distant..." -ForegroundColor Cyan
git remote -v

Write-Host ""
$currentRemote = git config --get remote.origin.url
Write-Host "Remote actuel : $currentRemote" -ForegroundColor White
$changeRemote = Read-Host "Voulez-vous pousser vers votre propre repo GitHub ? (o/n)"

if ($changeRemote -eq "o" -or $changeRemote -eq "O" -or $changeRemote -eq "y" -or $changeRemote -eq "Y") {
    $newRepo = Read-Host "Entrez l'URL de votre dépôt GitHub (ex: https://github.com/VOTRE_PSEUDO/BTT-Studio.git)"
    if ($newRepo) {
        git remote set-url origin $newRepo
        Write-Host "[✓] Remote 'origin' mis à jour vers $newRepo" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "Staging des fichiers..." -ForegroundColor Cyan
git add .

Write-Host "Création du commit de release v1.0.0..." -ForegroundColor Cyan
git commit -m "feat: Major v1.0.0 Release - Practice Mod 1.0 (1.3 UI, movable RGB input display, instant camera, reload at pos, Luigi 3D, GitHub Pages showcase)"

Write-Host ""
Write-Host "[✓] Commit créé avec succès !" -ForegroundColor Green
Write-Host ""
Write-Host "Pour publier sur GitHub, exécutez :" -ForegroundColor Yellow
Write-Host "  git push -u origin main" -ForegroundColor White
Write-Host "  git tag v1.0.0" -ForegroundColor White
Write-Host "  git push origin v1.0.0" -ForegroundColor White
Write-Host ""
Write-Host "GitHub Actions déploiera automatiquement :" -ForegroundColor Cyan
Write-Host "  1. La page vitrine interactive sur GitHub Pages" -ForegroundColor White
Write-Host "  2. La release officielle v1.0.0 avec les notes de release" -ForegroundColor White
Write-Host ""
