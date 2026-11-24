% Identificacion del banco de frenado con 3 respuestas al escalon (1 y 2 estiman, 3 valida)
archivos = {'escalon1.csv', 'escalon2.csv', 'escalon3.csv'};
Ts = 0.02;
d = cell(1, 3);

figure; hold on; grid on;
for k = 1:3
    datos = readtable(archivos{k});
    t = datos.t_s; u = datos.u_us; w = datos.rpm;
    pre = t < 1.0;
    du = u - mean(u(pre));
    dw = w - mean(w(pre));
    d{k} = iddata(dw, du, Ts, 'InputName', 'u servo', 'OutputName', 'omega');
    plot(t, w);
end
xline(1.0, '--k', 'escalon');
xlabel('t (s)'); ylabel('\omega (rpm)'); legend('Captura 1', 'Captura 2', 'Captura 3');
title('Respuestas al escalon medidas (u: 713 \rightarrow 733 \mus)');

est = merge(d{1}, d{2});
val = d{3};
A = max(abs(val.InputData));

% primer y segundo orden con retardo estimado
sys1 = tfest(est, 1, 0, NaN);
sys2 = tfest(est, 2, 0, NaN);

figure; compare(val, sys1, sys2); grid on;
title('Validacion con la captura 3');

figure; step(A * sys1, A * sys2); grid on; legend('1er orden', '2do orden');
title(sprintf('Respuesta al escalon del modelo (escalon de %d \\mus)', round(A)));

figure; impulse(sys1, sys2); grid on; legend('1er orden', '2do orden');
title('Respuesta al impulso del modelo');

K = dcgain(sys1);
tau = -1 / real(pole(sys1));
fprintf('\nModelo de 1er orden: G(s) = K e^{-Ls} / (tau s + 1)\n');
fprintf('K = %.3f rpm/us   tau = %.3f s   L = %.3f s\n\n', K, tau, sys1.IODelay);
sys1
sys2
