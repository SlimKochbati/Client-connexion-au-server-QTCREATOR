#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class Window : public QWidget
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();

private:
         // Déclaration de vos membres privés ici, si nécessaire.
};

#endif // WINDOW_H
