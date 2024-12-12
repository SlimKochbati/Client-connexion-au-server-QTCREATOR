#include <QtWidgets>
#include <QtNetwork>
#include "Client.h"

Client::Client(QWidget *parent)
    : QDialog(parent)
    , hostCombo(new QComboBox)//Liste déroulante
    , portLineEdit(new QLineEdit)
    , connectButton(new QPushButton(tr("Connexion")))
    , sendMessageButton(new QPushButton(tr("Envoyer le message")))
    , sendMessage(new QLineEdit)
    , messageDisplay(new QTextEdit)
    , tcpSocket(new QTcpSocket(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    in.setDevice(tcpSocket); // on associ le flux de donnees au socket TCP.
    in.setVersion(QDataStream::Qt_5_0); // On s'assure que la version du flux est compatible avec celle du serveur (version Qt 5.0).

    // Interface utilisateur
    setupUi(); // Appel de la fonction qui initialise l'interface graphique.

    // Connexion des différents signaux et slots.
    connect(tcpSocket, &QIODevice::readyRead, this, &Client::readMessage); // Lorsque le client reçoit un message, il appelle readMessage.
    connect(tcpSocket, &QTcpSocket::connected, this, &Client::onConnected); // Si la connexion au serveur est réussie, appelle onConnected.
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Client::displayError); // Si une erreur se produit, appelle displayError.
    connect(connectButton, &QPushButton::clicked, this, &Client::requestConnection); // Si l'utilisateur clique sur "Connexion", appelle requestConnection.
    connect(sendMessageButton, &QPushButton::clicked, this, &Client::sendMessageToServer); // Si l'utilisateur clique sur "Envoyer", appelle sendMessageToServer.

    setWindowTitle(tr("Client")); // Définit le titre de la fenêtre.
    portLineEdit->setValidator(new QIntValidator(1, 65535, this)); // Ajoute une validation pour que le port soit un nombre entre 1 et 65535.
    connectButton->setDefault(true); // Définit le bouton de connexion comme bouton par défaut (en appuyant sur "Entrée").
    connectButton->setEnabled(false); // Désactive le bouton de connexion tant que les champs ne sont pas remplis correctement.

    // Liste des hôtes disponibles (ici, on met une adresse IP fixe).
    QStringList hosts;
    hosts << "172.16.13.141";
    hostCombo->addItems(hosts); // Ajoute les hôtes à la liste déroulante.
    hostCombo->setEditable(true); // Permet à l'utilisateur de modifier l'adresse IP.

    // On connecte les champs de saisie aux fonctions pour activer ou désactiver le bouton de connexion.
    connect(portLineEdit, &QLineEdit::textChanged, this, &Client::enableConnectButton);
    connect(hostCombo, &QComboBox::editTextChanged, this, &Client::enableConnectButton);
}

void Client::setupUi() {
    auto mainLayout = new QVBoxLayout(this); // Crée un layout vertical principal pour la fenêtre.

    // Labels pour les champs de saisie.
    auto hostLabel = new QLabel(tr("&Nom du serveur:"));
    hostLabel->setBuddy(hostCombo); // Associe le label avec le champ de saisie (hostCombo).
    auto portLabel = new QLabel(tr("&Port:"));
    portLabel->setBuddy(portLineEdit); // Associe le label avec le champ de saisie (portLineEdit).

    // Création du layout de formulaire pour le nom du serveur et le port.
    auto formLayout = new QFormLayout;
    formLayout->addRow(hostLabel, hostCombo);
    formLayout->addRow(portLabel, portLineEdit);

    mainLayout->addLayout(formLayout); // Ajoute le formulaire au layout principal.
    mainLayout->addWidget(new QLabel(tr("Messages :"))); // Ajoute le label "Messages".
    mainLayout->addWidget(messageDisplay); // Ajoute la zone de texte pour afficher les messages.
    mainLayout->addWidget(new QLabel(tr("Envoyer un message :"))); // Ajoute le label pour envoyer un message.
    mainLayout->addWidget(sendMessage); // Ajoute le champ de saisie pour entrer le message.
    mainLayout->addWidget(sendMessageButton); // Ajoute le bouton pour envoyer le message.
    mainLayout->addWidget(connectButton); // Ajoute le bouton pour la connexion.

    messageDisplay->setReadOnly(true); // La zone de message est en lecture seule, l'utilisateur ne peut pas modifier le texte.
    setLayout(mainLayout); // Applique le layout à la fenêtre.

    // Augmente la taille de la fenêtre pour que l'interface soit bien visible.
    resize(400, 400);
}

void Client::requestConnection() {
    connectButton->setEnabled(false); // Désactive le bouton "Connexion" après avoir cliqué pour éviter de le cliquer plusieurs fois.
    tcpSocket->abort(); // Annule toute connexion en cours.
    tcpSocket->connectToHost(hostCombo->currentText(), portLineEdit->text().toInt()); // Tente de se connecter au serveur avec l'adresse et le port donnés.
}

void Client::displayError(QAbstractSocket::SocketError socketError) {
    // Affiche un message d'erreur selon le type d'erreur de socket.
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError: // Si le serveur a fermé la connexion, on ne fait rien.
        break;
    case QAbstractSocket::HostNotFoundError: // Si le serveur n'est pas trouvé, affiche un message d'information.
        QMessageBox::information(this, tr("Client Chat"),
                                 tr("Le serveur n'a pas été trouvé. Vérifiez le "
                                    "nom du serveur et les paramètres du port."));
        break;
    case QAbstractSocket::ConnectionRefusedError: // Si la connexion a été refusée, affiche un message d'information.
        QMessageBox::information(this, tr("Client Chat"),
                                 tr("La connexion a été refusée par le serveur. "
                                    "Assurez-vous que le serveur Fortune fonctionne, "
                                    "et vérifiez que le nom du serveur et les paramètres "
                                    "du port sont corrects."));
        break;
    default: // Si une autre erreur se produit, affiche l'erreur spécifique.
        QMessageBox::information(this, tr("Client Chat"),
                                 tr("L'erreur suivante s'est produite : %1.")
                                     .arg(tcpSocket->errorString()));
    }
    connectButton->setEnabled(true); // Réactive le bouton "Connexion" après l'affichage de l'erreur.
}

void Client::readMessage() {
    in.startTransaction(); // Commence une transaction pour lire les données reçues.
    QString message;
    in >> message; // Lit le message reçu.
    if (!in.commitTransaction()) // Si la transaction n'est pas réussie, on ne fait rien.
        return;

    // Affiche le message reçu du serveur dans la zone de texte.
    displayMessage("Serveur", message);
}

void Client::sendMessageToServer() {
    QString message = sendMessage->text(); // Récupère le message saisi par l'utilisateur.
    if (message.isEmpty()) { // Si le message est vide, on ne fait rien.
        return;
    }
    qDebug() << "Message envoyé (Client) : " << message; // Affiche le message dans la console de débogage.
    displayMessage("Vous", message); // Affiche le message envoyé dans la zone de texte.
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly); // Crée un flux de données pour envoyer le message.
    out.setVersion(QDataStream::Qt_5_0); // Assure que la version du flux est compatible avec celle du serveur.
    out << message; // Ajoute le message au flux.
    tcpSocket->write(block); // Envoie le message au serveur via le socket.
    sendMessage->clear(); // Vide le champ de saisie après l'envoi du message.
}

void Client::displayMessage(const QString &sender, const QString &message) {
    QString timeStamp = QDateTime::currentDateTime().toString("dd/MM/yyyy | hh:mm:ss"); // Récupère la date et l'heure actuelles.
    messageDisplay->append(QString("[%1] %2 : %3").arg(timeStamp, sender, message)); // Affiche le message dans la zone de texte avec un horodatage.
}

void Client::enableConnectButton() {
    // Active le bouton de connexion seulement si le nom du serveur et le port sont non vides.
    connectButton->setEnabled(!hostCombo->currentText().isEmpty() &&
                              !portLineEdit->text().isEmpty());
}

void Client::sessionOpened() {
    enableConnectButton(); // Lorsque la session est ouverte, on active le bouton de connexion.
}

void Client::onConnected() {
    QString serverIp = tcpSocket->peerAddress().toString(); // Récupère l'adresse IP du serveur auquel on est connecté.
    displayMessage("<LOGS>", tr("Connexion réussie sur le serveur (%1)").arg(serverIp)); // Affiche un message indiquant que la connexion a réussi.
}
